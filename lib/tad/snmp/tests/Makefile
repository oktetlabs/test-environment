
CC=gcc

all: snmp_test trap_test

SNMP_LIBS= -lsnmp

CFLAGS= -DCMU_COMPATIBLE


snmp_test: snmp_test.c
	$(CC) -g $(CFLAGS) -o $@ $< $(SNMP_LIBS)

trap_test: trap_test.c
	$(CC) -g $(CFLAGS) -o $@ $< $(SNMP_LIBS)

