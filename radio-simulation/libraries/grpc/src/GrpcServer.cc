#include "GrpcServer.h"

#include <memory>
#include <string>

std::unique_ptr<grpc::Server>
startGrpcServer(const std::string &p_serverAddress,
                NodeServiceImpl &p_service) {
  grpc::ServerBuilder serverBuilder;

  int boundPort = 0;

  serverBuilder.AddListeningPort(p_serverAddress,
                                 grpc::InsecureServerCredentials(), &boundPort);

  serverBuilder.RegisterService(&p_service);

  std::unique_ptr<grpc::Server> server = serverBuilder.BuildAndStart();

  if (server == nullptr || boundPort == 0) {
    return nullptr;
  }

  return server;
}