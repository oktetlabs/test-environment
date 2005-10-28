make -C ../security clean all
make -C ../common clean all
cc -c my_login.c
cc -o my_login my_login.o iscsi_portal_group.o ../common/common_functions.o ../security/security_functions.o ../common/target_negotiate.o -lpthread
