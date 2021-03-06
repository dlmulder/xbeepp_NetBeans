#include "Xbee.h"
#include "XbeeLogger.h"
#include "XbeeFrameCommand.h"
#include "XbeeFrameCommandResponse.h"
#include "XbeeFrameDataSample.h"
#include "XbeeFrameRemoteCommandResponse.h"
#include "XbeeCommandResponseImpl.h"
#include "XbeePort.h"
#include "XbeeCommandException.h"

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <unistd.h>

using namespace std;

Xbee::Xbee():name(addr.toString()),valueAP(0),valueAO(0),valueCE(0),valueID(0),valueVR(0),valueHV(0),valuePR(0),valueIR(0),valueIC(0),valueDH(0),valueDL(0),portFunction{},digitalValue(0),analogValue{0},initialized(false),lastCommand(nullptr),lastResponse(nullptr),allRequestsHandled(false)
{
}

Xbee::Xbee(XbeeAddress &_addr):addr(_addr),name(_addr.toString()),valueAP(0),valueAO(0),valueCE(0),valueID(0),valueVR(0),valueHV(0),valuePR(0),valueIR(0),valueIC(0),valueDH(0),valueDL(0),portFunction{},digitalValue(0),analogValue{0},initialized(false),lastCommand(nullptr),lastResponse(nullptr),allRequestsHandled(false)
{
}

Xbee::~Xbee()
{
    //dtor
}

XbeeAddress& Xbee::getAddress()
{
    return addr;
}

void Xbee::print()
{
    XbeeLogger &log = XbeeLogger::GetInstance();

    log.doLog(string("Address=") + addr.toString(), XbeeLogger::Severity::Info, "Xbee");

    stringstream ss;
    ss << "API enabled(AP)=" << (int)valueAP;
    log.doLog(ss.str(), XbeeLogger::Severity::Debug, "Xbee");

    ss.clear();
    ss.str("API output(AO)=");
    ss << (int)valueAO;
    log.doLog(ss.str(), XbeeLogger::Severity::Debug, "Xbee");

    ss.clear();
    ss.str("PAN ID=0x");
    ss << hex << (int)valueID;
    log.doLog(ss.str(), XbeeLogger::Severity::Debug, "Xbee");

    ss.clear();
    ss.str("Firmware version=0x");
    ss << hex << (int)valueVR;
    log.doLog(ss.str(), XbeeLogger::Severity::Debug, "Xbee");

    for(int i=0;i<XBEE_MAX_PORTS;i++)
        if ((int)portFunction[i] != 0)
        {
            ss.clear();
            ss.str(string());
            ss << XbeePort::getName(static_cast<XbeePort::pinID>(i)) << ":" << XbeePort::getFunctionName(portFunction[i]) << endl;
            log.doLog(ss.str(), XbeeLogger::Severity::Debug, "Xbee");
        }
}

void Xbee::setAddress(uint32_t hi, uint32_t lo, uint16_t net)
{
    addr.setAddress(hi, lo, net);
}

XbeeAddress Xbee::getDestinationAddress()
{
    return XbeeAddress(valueDH, valueDL, 0);
}

/*bool Xbee::getDigitalValue(uint8_t index)
{
    return digitalValue[index];
}

void Xbee::setDigitalValue(uint8_t index, bool value)
{
    digitalValue[index] = value;
}

uint16_t Xbee::getAnalogValue(uint8_t index)
{
    return analogValue[index];
}

void Xbee::setAnalogValue(uint8_t index, uint16_t value)
{
    analogValue[index] = value;
}
*/
XbeePort::pinFunction Xbee::getPortFunction(XbeePort::pinID port)
{
    uint8_t index = static_cast<uint8_t>(port);
    if (index < 0 || index >= XBEE_MAX_PORTS)
        throw invalid_argument("XBee port count limited to 20 - see xbee_port enum");

    return portFunction[index];
}

void Xbee::setPortFunction(XbeePort::pinID port, XbeePort::pinFunction cfg)
{
//    cout << "setting function on port " << (int)port << " to " << (int)cfg << endl;
    uint8_t index = static_cast<uint8_t>(port);
    if (index < 0 || index >= XBEE_MAX_PORTS)
        throw invalid_argument("XBee port count limited to 20 - see xbee_port enum");

    portFunction[index]=cfg;
}

string Xbee::getNetRoleString()
{
    switch (getNetRole())
    {
    case xbeeNetRole::Coordinator:
        return "Coordinator";
    case xbeeNetRole::Router:
        return "Router";
    case xbeeNetRole::EndDevice:
        return "End device";
    default:
        return "Unknown";
    }
}

xbeeNetRole Xbee::getNetRole()
{
//    uint8_t mod = valueVR/0x100;
//
//    switch (mod)
//    {
//    case 0x21:
//        return xbeeNetRole::Coordinator;
//    case 0x23:
//        return xbeeNetRole::Router;
//    case 0x29:
//        return xbeeNetRole::EndDevice;
//    }
//
//    if (getHWVersion() == xbeeVersion::S3B ||
//		getHWVersion() == xbeeVersion::S2C ||
//		getHWVersion() == xbeeVersion::S2CPro)
//        return valueCE?xbeeNetRole::Coordinator:xbeeNetRole::Router;
//
    return xbeeNetRole::Unknown;
}

xbeeVersion Xbee::getHWVersion()
{
    uint8_t mod = valueHV/0x100;

    switch(mod)
    {
    case 0x19:
        return xbeeVersion::S2;
    case 0x1A:
        return xbeeVersion::S2Pro;
    case 0x21:
        return xbeeVersion::S2CPro;
    case 0x22:
    case 0x2d:
    case 0x2e:
    case 0x30:
        return xbeeVersion::S2C;
    case 0x23:
        return xbeeVersion::S3B;
    }

    return xbeeVersion::Unknown;
}


string Xbee::getHWVersionString()
{
    stringstream ss;

    ss << "XBee" << (isHWProVersion()?"-Pro ":" ") << getHWType() << "(" << hex << valueHV << ")";

    return ss.str();
}

bool Xbee::isHWProVersion()
{
    uint8_t mod = valueHV/0x100;
    return mod == 0x1A || mod == 0x21;
}

std::string Xbee::getHWType()
{
    uint8_t mod = valueHV/0x100;

    switch(mod)
    {
    case 0x19:
    case 0x1A:
        return "S2";
    case 0x21:
    case 0x22:
    case 0x2d:
    case 0x2e:
    case 0x30:
        return "S2C";
    case 0x23:
        return "S3B";
    }

    return "XX";
}

void Xbee::initialize()
{
	allRequestsHandled = false;
    try
    {
        updateState();
    } catch (exception &e) {
        XbeeLogger::GetInstance().doLog(string("Cant initialize ") + getName() + ":" + e.what(), XbeeLogger::Severity::Warning, "XbeeLocal");
    }

    initialized = true;

}

void Xbee::updateState()
{
//    typedef XbeeCommand::returnType rt;

    vector<string> cmds;

    cmds.push_back(XBEE_CMD_AP);
    cmds.push_back(XBEE_CMD_AO);
    cmds.push_back(XBEE_CMD_ID);
    cmds.push_back(XBEE_CMD_VR);
    cmds.push_back(XBEE_CMD_HV);
    cmds.push_back(XBEE_CMD_DH);
    cmds.push_back(XBEE_CMD_DL);
    cmds.push_back(XBEE_CMD_IR);
    cmds.push_back(XBEE_CMD_IC);

    for (auto it=cmds.begin(); it!=cmds.end(); it++)
    {
        queryValue(*it);
        usleep(20*1000);
    }

//    cout << "HW version:" << (int)getHWVersion() << endl;
//    if (getHWVersion() == xbeeVersion::S2C || getHWVersion() == xbeeVersion::S2CPro)
    queryValue(XBEE_CMD_CE);

//    typedef XbeePort::pinID pt;

//    usleep(1000*1000);

    vector<XbeePort::pinID> pins = getPins();

    for (auto it=pins.begin(); it!=pins.end(); it++)
    {
        updatePortConfig(*it);
        usleep(20*1000);
    }

    usleep(1000*1000);

/*    updatePortConfig(pt::P0);
    updatePortConfig(pt::P1);
    updatePortConfig(pt::P2);
    updatePortConfig(pt::D0);
    updatePortConfig(pt::D1);
    updatePortConfig(pt::D2);
    updatePortConfig(pt::D3);
    updatePortConfig(pt::D4);
    updatePortConfig(pt::D5);*/

/*   These ports are not available on XBee24-ZB

    updatePortConfig(XBEE_CMD_P3, pt::P3);
    updatePortConfig(XBEE_CMD_P4, pt::P4);
    updatePortConfig(XBEE_CMD_P5, pt::P5);
    updatePortConfig(XBEE_CMD_P6, pt::P6);
    updatePortConfig(XBEE_CMD_P7, pt::P7);
    updatePortConfig(XBEE_CMD_P8, pt::P8);
    updatePortConfig(XBEE_CMD_P9, pt::P9);
    updatePortConfig(XBEE_CMD_D8, pt::D8);*/
}

bool Xbee::areAllRequestsHandled()
{
	return allRequestsHandled;
}

void Xbee::updatePortConfig(XbeePort::pinID p)
{
    try
    {
        queryValue(p);
    } catch (invalid_argument &iaex)
    {
        XbeeLogger::GetInstance().doLog(string("Cant get port configuration:") + XbeePort::getName(p), XbeeLogger::Severity::Warning, "XbeeLocal");
    }
}

void Xbee::callHandler(uint8_t frmId, XbeeCommandResponse &resp)
{
    lock_guard<mutex> blwr(mCallback);

    if (callbackHandlers.count(frmId) != 0)
    {
//        cout << "Call handler " << resp.getCommandString() << endl;
        response_handler &hnd = callbackHandlers[frmId];

        hnd.cb(this, hnd.cmd, resp);

        callbackHandlers.erase(frmId);
		
		if (callbackHandlers.empty())
			allRequestsHandled = true;
    }
}

void Xbee::setHandler(uint8_t frmId, response_handler &hnd)
{
    lock_guard<mutex> blwr(mCallback);

    if (callbackHandlers.count(frmId))
    {
        stringstream ss;
        ss << "Previous response not consumed for frame ID " << (int)frmId << " for " << getName();
        XbeeLogger::GetInstance().doLog(ss.str(), XbeeLogger::Severity::Warning, "Xbee response handler");
    }

    callbackHandlers[frmId] = hnd;
}

void Xbee::setResponse(Xbee *xbee, XbeeCommand &cmd, XbeeCommandResponse &resp)
{
//    cout << "setResponse " << xbee->getAddress().toString() << " " << resp.getCommandString() << " on " << xbee->getAddress().toString() << endl;
    if (xbee->lastResponse != nullptr)
    {
        stringstream ss;
        ss << "last response not consumed yet on " << xbee->getAddress().toString();
        throw runtime_error(ss.str());
    }

//    cout << "setResponse2 " << xbee->getAddress().toString() << " " << resp.getCommandString() << endl;
    xbee->lastResponse = new XbeeCommandResponseImpl(resp);
    xbee->lastCommand = new XbeeCommand(cmd);
//    cout << "setResponse3 " << xbee->getAddress().toString() << " " << resp.getCommandString() << endl;

    stringstream ss;

    ss << "command response on " << xbee->getName() << " to command " << xbee->lastResponse->getCommandString();
    XbeeLogger::GetInstance().doLog(ss.str(), XbeeLogger::Severity::Info, "Xbee response handler");
}

uint64_t Xbee::sendCommandWithResponseSync(XbeeCommand &cmd, XbeeCommand::returnType rt)
{
    lock_guard<mutex> guard(mResponse);

//    cout << "sendCommandWithResponseSync" << endl;

    lastResponse = nullptr;
    lastCommand = nullptr;

    response_handler h = { setResponse, cmd };
    sendCommand(cmd.getCommand(), h);

    return waitforResponseWithCleanup();
}

void Xbee::sendCommandWithResponseSync(XbeeCommand &cmd, uint8_t buf[], size_t buflen)
{
    lock_guard<mutex> guard(mResponse);

//    cout << "sendCommandWithResponseSync raw" << endl;

    lastResponse = nullptr;
    lastCommand = nullptr;

    response_handler h = { setResponse, cmd } ;
    sendCommand(cmd.getCommand(), h);

    waitforResponse();
    lastResponse->getRawValue(buf, buflen);

    cleanupResponse();
}

uint64_t Xbee::sendCommandWithResponseSync(XbeeCommand &cmd, uint8_t param, XbeeCommand::returnType rt)
{
    lock_guard<mutex> guard(mResponse);

    response_handler h = { setResponse , cmd };

//    cout << "send command" << endl;
    sendCommand(cmd.getCommand(), param, h);

//    cout << "waiting for response" << endl;
    return waitforResponseWithCleanup();
}

void Xbee::waitforResponse()
{
    for(int i=0; i<20;i++)
    {
        usleep(100*1000);

//        cout << "waitforResponse try " << i << " " << lastResponse << " on " << getAddress().toString() << endl;
        if (lastResponse != nullptr)
            break;
    }

    if (lastResponse == NULL)
        throw runtime_error("wait");
}

void Xbee::cleanupResponse()
{
//    cout << "cleanup response" << endl;
    delete lastCommand;
    delete lastResponse;
    lastResponse = nullptr;
    lastCommand = nullptr;
}

uint64_t Xbee::waitforResponseWithCleanup()
{
//    cout << "wait for response" << endl;
    waitforResponse();

    if (lastResponse->getStatus() != XbeeCommandResponse::status::OK)
        throw XbeeCommandException(lastResponse->getStatus());
    uint64_t val = lastResponse->getValue();

//    cout << "cleanup" << endl;
    cleanupResponse();
    return val;
}

void Xbee::configurePortFunction(XbeePort::pinID port, XbeePort::pinFunction fnc)
{
    try
    {
        XbeeCommand cmd(port, fnc);

//        cout << " configurePortFunction " << endl;
        sendCommandWithResponseSync(cmd, static_cast<uint8_t>(fnc), XbeeCommand::returnType::NONE);
//        setPortFunction(port, fnc);

        stringstream ss;
        ss << "Port " << XbeePort::getName(cmd.getPort()) << " configured as " << XbeePort::getFunctionName(cmd.getPortFunction()) << " successfully on " << getName();
        XbeeLogger::GetInstance().doLog(ss.str(), XbeeLogger::Severity::Info, "XbeeLocal");
    } catch (XbeeException &ex) {
        throw ex;
    } catch (exception &e) {
        stringstream ss;
        ss << "Port " << XbeePort::getName(port) << " could not be configured as " << XbeePort::getFunctionName(fnc) << " on " << getName() << ":" << e.what();
        XbeeLogger::GetInstance().doLog(ss.str(), XbeeLogger::Severity::Warning, "XbeeLocal");
    }
}

vector<XbeePort::pinID> Xbee::getPins()
{

    return XbeePort::getPins(getHWVersion());
}

bool Xbee::getPullupPin(XbeePort::pinID port)
{
    throw runtime_error("not implemented");
}

bool Xbee::setValueByCommand(XbeeCommand &cmd, XbeeCommandResponse &resp)
{
    if (cmd.getCommand() == XBEE_CMD_AP)
    {
        setValueAP(resp.getByteValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_AO)
    {
        setValueAO(resp.getByteValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_ID)
    {
        setValueID(resp.getShortValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_VR)
    {
        setValueVR(resp.getShortValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_HV)
    {
        setValueHV(resp.getShortValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_PR)
    {
        setValuePR(resp.getShortValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_DH)
    {
        setValueDH(resp.getWordValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_DL)
    {
        setValueDL(resp.getWordValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_IR)
    {
        setValueIR(resp.getShortValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_IC)
    {
        setValueIC(resp.getShortValue());
        return true;
    }

    if (cmd.getCommand() == XBEE_CMD_CE)
    {
        setValueCE(resp.getByteValue());
        return true;
    }

    return false;
}
