# Makefile

flags?="-pthread"

libraryFileName?="mq.cpp"
serverFileName?="tcp_server.cpp"
clientProdFileName?="tcp_client_producent.cpp"
clientConsFileName?="tcp_client_consumer.cpp"

serverOutputName?="server"
clientProdOutputName?="client_prod"
clientConsOutputName?="client_cons"

main: clean
	@echo "Compiling.."
	g++ ${libraryFileName} ${serverFileName} -o ${serverOutputName} ${flags}
	g++ ${libraryFileName} ${clientProdFileName} -o ${clientProdOutputName} ${flags}
	g++ ${libraryFileName} ${clientConsFileName} -o ${clientConsOutputName} ${flags}
	@echo "Compilation succesfull!"
	@echo "..2 output files"

clean:
	@echo "Cleaning directory from executable files."
	rm -f ${serverOutputName}
	rm -f ${clientOutputName}
