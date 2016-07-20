webserv: socklib.c webserv.c
	cc webserv.c socklib.c -lpthread -o webserv
