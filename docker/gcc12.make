# ----------------------------------------------------------------------------------
# make -f docker/gcc12.make -n
# make -f docker/gcc12.make all
# make -f docker/gcc12.make boost fmt spdlog
# ----------------------------------------------------------------------------------

.PHONY: all gcc12-init boost fmt spdlog rapidjson grpc uwebsockets libdatachannel sqlcipher pqxx mongocxx redis amqp-cpp aws-sdk-cpp gcc12-final

# Default target to build everything
all: gcc12-init boost fmt spdlog rapidjson grpc uwebsockets libdatachannel sqlcipher pqxx mongocxx redis amqp-cpp aws-sdk-cpp gcc12-final

# Build the gcc12-init-build target
gcc12-init:
	docker build -f docker/gcc12.docker --target gcc12-init-build -t gcc12-init-image .

# Build the boost-build target
boost:
	docker build -f docker/gcc12.docker --target boost-build -t boost-image .

# Build the fmt-build target
fmt:
	docker build -f docker/gcc12.docker --target fmt-build -t fmt-image .

# Build the spdlog-build target
spdlog:
	docker build -f docker/gcc12.docker --target spdlog-build -t spdlog-image .

# Build the rapidjson-build target
rapidjson:
	docker build -f docker/gcc12.docker --target rapidjson-build -t rapidjson-image .

# Build the uwebsockets-build target
uwebsockets:
	docker build -f docker/gcc12.docker --target uwebsockets-build -t uwebsockets-image .

# Build the grpc-build target
grpc:
	docker build -f docker/gcc12.docker --target grpc-build -t grpc-image .

# Build the libdatachannel-build target
libdatachannel:
	docker build -f docker/gcc12.docker --target libdatachannel-build -t libdatachannel-image .

# Build the sqlcipher-build target
sqlcipher:
	docker build -f docker/gcc12.docker --target sqlcipher-build -t sqlcipher-image .

# Build the pqxx-build target
pqxx:
	docker build -f docker/gcc12.docker --target pqxx-build -t pqxx-image .

# Build the mongocxx-build target
mongocxx:
	docker build -f docker/gcc12.docker --target mongocxx-build -t mongocxx-image .

# Build the redis-build target
redis:
	docker build -f docker/gcc12.docker --target redis-build -t redis-image .

# Build the amqp-cpp-build target
amqp-cpp:
	docker build -f docker/gcc12.docker --target amqp-cpp-build -t amqp-cpp-image .

# Build the aws-sdk-cpp-build target
aws-sdk-cpp:
	docker build -f docker/gcc12.docker --target aws-sdk-cpp-build -t aws-sdk-cpp-image .

# Build the triton-client-build target
triton-client:
	docker build -f docker/gcc12.docker --target triton-client-build -t triton-client-image .

# Build the final-build target
gcc12-final:
	docker build -f docker/gcc12.docker --target gcc12-final-build -t gcc12-final-image .

