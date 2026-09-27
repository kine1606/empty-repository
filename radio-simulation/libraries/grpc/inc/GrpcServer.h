#ifndef GRPC_SERVER_H
#define GRPC_SERVER_H

#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "NodeServiceImpl.h"

/**
 * @brief Creates and starts a gRPC server for a mailbox service.
 *
 * The returned server listens on @p p_serverAddress and dispatches requests to
 * @p p_service. The caller owns the server and must keep the service alive for
 * at least as long as the server runs.
 *
 * @param p_serverAddress Address on which the server listens.
 * @param p_service Service implementation registered with the server.
 * @return A running server, or nullptr if the server could not be started.
 */
std::unique_ptr<grpc::Server>
startGrpcServer(const std::string &p_serverAddress, NodeServiceImpl &p_service);

#endif // GRPC_SERVER_H
