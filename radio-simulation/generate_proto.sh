#!/bin/bash
 
set -e
 
PROTO_FILE="libraries/proto/mailbox.proto"
PROTO_DIR="libraries/proto"
 
echo "[INFO] Generating protobuf files..."
 
protoc \
    --proto_path="$PROTO_DIR" \
    --cpp_out="$PROTO_DIR" \
    --grpc_out="$PROTO_DIR" \
    --plugin=protoc-gen-grpc="$(which grpc_cpp_plugin)" \
    "$PROTO_FILE"
 
echo "[SUCCESS] Protobuf generation completed."