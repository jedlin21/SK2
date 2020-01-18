# Makefile

flags?="-pthread"

libraryFileName?="mq.cpp"
serverFileName?="tcp_server.cpp"
clientFileName?="tcp_client.cpp"

serverOutputName?="server"
clientOutputName?="client"

main: clean
	@echo "Compiling.."
	g++ ${libraryFileName} ${serverFileName} -o ${serverOutputName} ${flags}
	g++ ${libraryFileName} ${clientFileName} -o ${clientOutputName} ${flags}
	@echo "Compilation succesfull!"
	@echo "..2 output files"

clean:
	@echo "Cleaning directory from executable files."
	rm -f ${serverOutputName}
	rm -f ${clientOutputName}
