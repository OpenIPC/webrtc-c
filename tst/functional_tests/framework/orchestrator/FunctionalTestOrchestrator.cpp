//
// Created by Katey, Anurag on 3/16/22.
//

#include "FunctionalTestOrchestrator.h"
#include "../utils/Utils.h"
#include <cassert>

/*************************************************************************************
 *                                     PRIVATE APIS                                  *
 *************************************************************************************/

int FunctionalTestOrchestrator::createTestDirectory() {
    string command = "bash ";
    command.append(SETUP_TEST_DIRECTORY_SCRIPT);
    command.append(" ");
    command.append(functionalTestConfig.testArtifactsDirectory);
    cout << "\n createTestDirectory command : " << command;
    return Utils::executeBashCommand(command);
}

int setUpPeerNetwork() {
    return Utils::executeBashCommand(DOCKER_NETWORK_COMMAND);
}

int FunctionalTestOrchestrator::launchContainer(const string& containerName,
                                                const string& containerIp,
                                                const string& network,
                                                const string& portForwardingMapping) {
    // docker container ls -a | grep viewer-alpineee
    string command = "docker run -t -d -P ";
    if (!portForwardingMapping.empty()) {
        command += " -p " + portForwardingMapping;
    }
    command += " --net=" + network;
    command += " --ip " + containerIp;
    // docker run -t -d -P -v ~/kvs_git_workspace:/tmp/xyz --name viewer-alpine webrtc-alpine-image
    command += " -v " + TEST_SOURCE_CODE_ROOT + ":" + "/tmp/test-sdk-source/";
    command += " -v " + functionalTestConfig.testArtifactsLoggingDirectory + ":" + CONTAINER_LOG_DIRECTORY;
    command += " -v " + SCRIPTS_ROOT + ":" + "/tmp/scripts/";
    command += " --name " + containerName;
    command += " " + functionalTestConfig.containerImageName;

    cout << "\n launching container << " << containerName << "  command : " << command;
    return Utils::executeBashCommand(command);
}

int FunctionalTestOrchestrator::launchMasterContainer() {
    return launchContainer(functionalTestConfig.masterContainerName, MASTER_IP, DOCKER_NETWORK_NAME, "");
}

int FunctionalTestOrchestrator::launchViewerContainer() {
    return launchContainer(functionalTestConfig.viewerContainerName, VIEWER_IP, DOCKER_NETWORK_NAME, "");
}

int FunctionalTestOrchestrator::launchSignalingControlPlaneServerContainer() {
    string portForwardingMapping = SIGNALING_CP_SERVER_PORT + ":" + SIGNALING_CP_SERVER_PORT;
    return launchContainer(functionalTestConfig.signalingCPServerContainerName, SIGNALING_CP_SERVER_IP, DOCKER_NETWORK_NAME, portForwardingMapping);
}

int FunctionalTestOrchestrator::launchSignalingDataPlaneServerContainer() {
    string portForwardingMapping = SIGNALING_DP_SERVER_PORT + ":" + SIGNALING_DP_SERVER_PORT;
    return launchContainer(functionalTestConfig.signalingDPServerContainerName, SIGNALING_DP_SERVER_IP, DOCKER_NETWORK_NAME, portForwardingMapping);
}

string FunctionalTestOrchestrator::getSignalingControlPlaneServerPid() {
    cout << "\n getSignalingControlPlaneServerPid command : " << GET_SIGNALING_CONTROL_PLANE_SERVER_PID_COMMAND;
    return Utils::executeBashCommand(GET_SIGNALING_CONTROL_PLANE_SERVER_PID_COMMAND.c_str());
}

string FunctionalTestOrchestrator::getSignalingDataPlaneServerPid() {
    cout << "\n getSignalingDataPlaneServerPid command : " << GET_SIGNALING_DATA_PLANE_SERVER_PID_COMMAND;
    return Utils::executeBashCommand(GET_SIGNALING_DATA_PLANE_SERVER_PID_COMMAND.c_str());
}

int FunctionalTestOrchestrator::syncSourceCode() {
    string syncSourceCommand = "bash " + SYNC_SOURCE_CODE_WITH_CONTAINER;
    syncSourceCommand += " " + functionalTestConfig.sourceRepoPath;
    syncSourceCommand += " " + functionalTestConfig.repoName;
    syncSourceCommand += " " + TEST_SOURCE_CODE_ROOT;

    cout << "\n syncSourceCode command : " << syncSourceCommand;
    return Utils::executeBashCommand(syncSourceCommand);
}

int FunctionalTestOrchestrator::buildSourceCodeOnMasterContainer(bool cleanBuild) {
    //  master-container-test-1647823714  /bin/bash /tmp/scripts/sdk_container_build.sh

    string command = "docker exec --privileged -it ";
    command += functionalTestConfig.masterContainerName;
    command += " /bin/bash ";
    command += BUILD_WEBRTC_SDK_SOURCE;
    command += " ";
    command += TEST_SOURCE_CODE_CONTAINER_MAPPED_DIR;
    command += functionalTestConfig.repoName;
    if (cleanBuild) {
        command += " --clean_build";
    }

    cout << "\n buildSourceCodeOnMasterContainer command : " << command;
    return Utils::executeBashCommand(command);
}

/*************************************************************************************
 *                                      PUBLIC APIS                                  *
 *************************************************************************************/

FunctionalTestOrchestrator::FunctionalTestOrchestrator(FunctionalTestOrchestratorConfiguration& functionalTestConfig) {
    this->functionalTestConfig = functionalTestConfig;
}

int FunctionalTestOrchestrator::setupTestResources() {
    // Start signaling CP server

    assert(ensureNoTestResourcesRunning());

    int status = 0;
    cout << "\n\n ==== Creating Test Directories ====";
    createTestDirectory();
    cout << "\n\n ==== Syncing Source Code ====";
    syncSourceCode();

    setUpPeerNetwork();
    cout << "\n\n ==== Launching CP Signaling Server ====";
    assert(0 == launchSignalingControlPlaneServerContainer());
    cout << "\n\n ==== Launching DP Signaling Server ====";
    assert(0 == launchSignalingDataPlaneServerContainer());
    cout << "\n\n ==== Launching Master Container ====";
    assert(0 == launchMasterContainer());
    cout << "\n\n ==== Launching Viewer Container ====";
    assert(0 == launchViewerContainer());

    return 0;
}

int FunctionalTestOrchestrator::launchSignalingControlPlaneServer() {
    string signalingControlPlaneServerScript = " /bin/bash";
    signalingControlPlaneServerScript += " " + SETUP_MOCK_CONTROL_PLANE_SERVER_SCRIPT;
    signalingControlPlaneServerScript += " " + SCRIPTS_ROOT;
    signalingControlPlaneServerScript += " " + CONTAINER_LOG_DIRECTORY;
    signalingControlPlaneServerScript += " " + SIGNALING_CP_SERVER_IP;
    signalingControlPlaneServerScript += " " + SIGNALING_CP_SERVER_PORT;

    string dockerCommand = DOCKER_EXEC_COMMAND;
    dockerCommand += " " + functionalTestConfig.signalingCPServerContainerName;
    dockerCommand += " " + signalingControlPlaneServerScript;

    cout << "\n\n ==== Starting control plane server ==== " << dockerCommand;
    return Utils::executeBashCommand(dockerCommand);
}

int FunctionalTestOrchestrator::launchSignalingDataPlaneServer() {
    string signalingDataPlaneServerScript = " /bin/bash";
    signalingDataPlaneServerScript += " " + SETUP_MOCK_DATA_PLANE_SERVER_SCRIPT;
    signalingDataPlaneServerScript += " " + SCRIPTS_ROOT;
    signalingDataPlaneServerScript += " " + CONTAINER_LOG_DIRECTORY;
    signalingDataPlaneServerScript += " " + SIGNALING_DP_SERVER_IP;
    signalingDataPlaneServerScript += " " + SIGNALING_DP_SERVER_PORT;

    string dockerCommand = DOCKER_EXEC_COMMAND;
    dockerCommand += " " + functionalTestConfig.signalingDPServerContainerName;
    dockerCommand += " " + signalingDataPlaneServerScript;

    cout << "\n\n ==== Starting data plane server ==== " << dockerCommand;
    return Utils::executeBashCommand(dockerCommand);
}

int FunctionalTestOrchestrator::shutDownSignalingControlPlaneServer() {
    string shutDownSignalingControlPlaneServer = SIGNALING_SERVER_PROTOCOL_PREFIX;
    shutDownSignalingControlPlaneServer += SIGNALING_CP_SERVER_IP;
    shutDownSignalingControlPlaneServer += ":" + SIGNALING_CP_SERVER_PORT;
    shutDownSignalingControlPlaneServer += "/shutdown";

    string dockerCommand = DOCKER_EXEC_COMMAND;
    dockerCommand += " " + functionalTestConfig.signalingCPServerContainerName;
    dockerCommand += " " + shutDownSignalingControlPlaneServer;

    cout << "\n\n ==== Stopping control plane server ==== " << dockerCommand;
    return Utils::executeBashCommand(dockerCommand);
}

int FunctionalTestOrchestrator::shutDownSignalingDataPlaneServer() {
    string dockerCommand = DOCKER_EXEC_COMMAND;
    dockerCommand += " " + functionalTestConfig.signalingCPServerContainerName;
    dockerCommand += " " + KILL_SIGNALING_DP_SERVER_PROCESS;

    cout << "\n\n ==== Stopping data plane server ==== " << dockerCommand;
    return Utils::executeBashCommand(dockerCommand);
}

void FunctionalTestOrchestrator::cleanTestResources() {
    Utils::executeBashCommand(KILL_CONTAINER_COMMAND_PREFIX + functionalTestConfig.masterContainerName);
    Utils::executeBashCommand(KILL_CONTAINER_COMMAND_PREFIX + functionalTestConfig.viewerContainerName);
    Utils::executeBashCommand(KILL_CONTAINER_COMMAND_PREFIX + functionalTestConfig.signalingCPServerContainerName);
    Utils::executeBashCommand(KILL_CONTAINER_COMMAND_PREFIX + functionalTestConfig.signalingDPServerContainerName);

    Utils::executeBashCommand(DELETE_DOCKER_NETWORK_COMMAND);
}

bool FunctionalTestOrchestrator::checkAllResourcesRunning() {

    auto masterContainerPid = processManager.getContainerPid(functionalTestConfig.masterContainerName);
    auto viewerContainerPid = processManager.getContainerPid(functionalTestConfig.viewerContainerName);
    auto signalingCPServerContainerPid = processManager.getContainerPid(functionalTestConfig.signalingCPServerContainerName);
    auto signalingDPServerContainerPid = processManager.getContainerPid(functionalTestConfig.signalingDPServerContainerName);

    cout << "\n Process PIDs:";
    cout << "\n masterContainerPid: " << masterContainerPid;
    cout << "\n viewerContainerPid: " << viewerContainerPid;
    cout << "\n signalingCPServerContainerPid: " << signalingCPServerContainerPid;
    cout << "\n signalingDPServerContainerPid: " << signalingDPServerContainerPid;

    return !masterContainerPid.empty() && !viewerContainerPid.empty() &&
        !signalingCPServerContainerPid.empty() && !signalingDPServerContainerPid.empty();
}

bool FunctionalTestOrchestrator::ensureNoTestResourcesRunning() {

    auto masterContainerPid = processManager.getContainerPid(functionalTestConfig.masterContainerName);
    auto viewerContainerPid = processManager.getContainerPid(functionalTestConfig.viewerContainerName);
    auto signalingCPServerContainerPid = processManager.getContainerPid(functionalTestConfig.signalingCPServerContainerName);
    auto signalingDPServerContainerPid = processManager.getContainerPid(functionalTestConfig.signalingDPServerContainerName);

    cout << "\n Process PIDs:";
    cout << "\n masterContainerPid: " << masterContainerPid;
    cout << "\n viewerContainerPid: " << viewerContainerPid;
    cout << "\n signalingCPServerContainerPid: " << signalingCPServerContainerPid;
    cout << "\n signalingDPServerContainerPid: " << signalingDPServerContainerPid;

    return masterContainerPid.empty() && viewerContainerPid.empty() &&
        signalingCPServerContainerPid.empty() && signalingDPServerContainerPid.empty();
}

int FunctionalTestOrchestrator::startMasterApplication() {
    return 0;
}

int FunctionalTestOrchestrator::startViewerApplication() {
    return 0;
}

int FunctionalTestOrchestrator::setupResourceMonitoringThread() {
    return 0;
}

int FunctionalTestOrchestrator::waitForTestToTerminate() {
    return 0;
}

int FunctionalTestOrchestrator::validateMetrics() {
    return 0;
}
int FunctionalTestOrchestrator::uploadTestArtifacts() {
    return 0;
}

int FunctionalTestOrchestrator::cleanUpTestArtifacts() {
    return 0;
}

int FunctionalTestOrchestrator::isolateMasterAndViewerNetwork() {
    return 0;
}