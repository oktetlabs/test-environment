all: userland_lib security common my_login.o iscsi_portal_group.o target_lib.o

rebuild:
	make clean all

target_lib.o: my_login.o security common userland_lib
	ld -r -o target_lib.o my_login.o iscsi_portal_group.o ../common/common_functions.o ../security/security_functions.o ../common/target_negotiate.o
#my_login: my_login.o security common userland_lib
#	cc -o my_login my_login.o iscsi_portal_group.o ../common/common_functions.o ../security/security_functions.o ../common/target_negotiate.o -lpthread

my_login.o: iscsi_portal_group.o my_login.c
	cc -c my_login.c

iscsi_portal_group.o:
	cc -c iscsi_portal_group.c

security:
	make -C ../security all

common:
	make -C ../common all

userland_lib:
	make -C ../userland_lib

clean:
	make -C ../security clean
	make -C ../common clean
	make -C ../userland_lib clean
	rm *.o;

clean_s:
	make -C ../security clean

clean_c:
	make -C ../common clean

clean_u:
	make -C ../userland_lib clean

clean_l:
	rm my_login.o;
