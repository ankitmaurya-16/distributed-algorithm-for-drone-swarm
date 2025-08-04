// Ankit's Part

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include <Arduino.h>
#include <map>
#include <vector>
#include <set>
#include "../include/utilities/time_utils.h"

struct NodeInfo
{
    int nodeId;
    unsigned long lastHeartbeatTime;
    unsigned long lastSequenceNumber;
    bool isAlive;
    int timeoutId;
    String nodeName;
};

struct HeartbeatMessage
{
    int fromNodeId;
    unsigned long timestamp;
    unsigned long sequenceNumber;
    int messageType = 1;
};

class HeartbeatSystem
{
private:
    TimeoutManager timeoutManager;
    std::map<int, NodeInfo> knownNodes;
    std::set<int> failedNodes;

    int myNodeId;
    unsigned long heartbeatInterval;
    unsigned long failureTimeout;
    unsigned long lastHeartbeatSent;
    unsigned long mySequenceNumber;

    int totalHeartbeatsSent;
    int totalHeartbeatsReceived;
    int totalFailuresDetected;

public:
    HeartbeatSystem(int nodeId, unsigned long interval = 1000, unsigned long timeout = 3000);

    void sendHeartbeat();
    bool receiveHeartbeat(int fromNodeId, unsigned long timestamp, unsigned long sequenceNumber);
    void receiveHeartbeat(const HeartbeatMessage &msg);

    void addNode(int nodeId);
    void removeNode(int nodeId);
    bool isNodeAlive(int nodeId);
    std::vector<int> getAliveNodes();
    std::vector<int> getFailedNodes();

    std::vector<int> checkFailedNodes();
    void processHeartbeats();

    void printStatus();
    void printDetailedStatus();
    HeartbeatMessage createHeartbeatMessage();

    int getMyNodeId() const { return myNodeId; }
    int getAliveNodeCount();
    int getTotalNodesCount();

    void printStatistics();
    void resetStatistics();
};

#endif