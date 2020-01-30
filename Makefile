# Makefile

flags?=-pthread
libflag?=-L. -l_mq
lib2flag?=-Wl,-rpath,$$ORIGIN

libraryImplementationFileName?="mq_impl.cpp"
libraryFileName?="lib_mq.cpp"

libraryOutputTemporary?="lib_mq.so"
libraryOutputFileName?="lib_mq.a"

serverFileName?="tcp_server.cpp"
clientProdFileName?="tcp_client_producent.cpp"
clientConsFileName?="tcp_client_consumer.cpp"

serverOutputName?="server"
clientProdOutputName?="client_prod"
clientConsOutputName?="client_cons"

all: main

main: clean libraryCompile
	@echo "Compiling.."
	g++ ${serverFileName} ${libflag} ${lib2flag} -o ${serverOutputName}
	g++ -o ${clientProdOutputName} ${libflag} ${lib2flag} ${clientProdFileName} 
	g++ -o ${clientConsOutputName} ${libflag} ${lib2flag} ${clientConsFileName} 
	@echo "Compilation succesfull!"
	@echo "...4 output files"

libraryCompile: cleanLibrary
	@echo "Compiling library"
	g++ -shared -fPIC -o ${libraryOutputTemporary} ${libraryImplementationFileName} ${libraryFileName} ${flags}
	ar rvs ${libraryOutputFileName} ${libraryOutputTemporary}

clean:
	@echo "Cleaning directory from executable files."
	rm -f ${serverOutputName}
	rm -f ${clientProdOutputName}
	rm -f ${clientConsOutputName}
	
cleanLibrary:
	rm -f ${libraryOutputTemporary}
	rm -f ${libraryOutputFileName}

