CC=g++
CFLAGS=-Wall -Werror -g -I/usr/local/include
LDFLAGS=-lynet -lythread
TOOL_LDFLAGS=$(LDFLAGS) -lgflags

all: libbtunnel.a register_service bproxy bproxy_server bproxy_client \
     echo_server echo_client

.SUFFIXES: .cpp

.cpp.o:
	$(CC) $(CFLAGS) -c $<

libbtunnel.a: util.o registration.o service.o browse.o peer.o \
              encoding.o peer_message.o exported_service.o local_service.o \
              core.o
	$(AR) rc $@ $^

%_test: %_test.o test_util.o libbtunnel.a
	$(CC) -o $@ $^ $(LDFLAGS)

register_service: register_service.o libbtunnel.a
	$(CC) -o $@ $^ $(TOOL_LDFLAGS)

bproxy: bproxy.o libbtunnel.a
	$(CC) -o $@ $^ $(TOOL_LDFLAGS)

bproxy_server: bproxy_server.o libbtunnel.a
	$(CC) -o $@ $^ $(TOOL_LDFLAGS)

bproxy_client: bproxy_client.o libbtunnel.a
	$(CC) -o $@ $^ $(TOOL_LDFLAGS)

echo_server: echo_server.o libbtunnel.a
	$(CC) -o $@ $^ $(TOOL_LDFLAGS)

echo_client: echo_client.o libbtunnel.a
	$(CC) -o $@ $^ $(TOOL_LDFLAGS)


test: peer_message_test encoding_test exported_service_test core_test
	./encoding_test
	./peer_message_test
	./exported_service_test
	./core_test

clean:
	rm -f *.o *.a core *_test register_service bproxy \
		bproxy_server bproxy_client echo_server echo_client
