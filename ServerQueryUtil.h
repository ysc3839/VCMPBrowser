#pragma once

SOCKET g_UDPSocket;

void SendQuery(serverAddress address, char opcode)
{
	struct sockaddr_in sendaddr = { AF_INET };
	sendaddr.sin_addr.s_addr = address.ip;
	sendaddr.sin_port = htons(address.port);

	char *cip = (char *)&(address.ip);
	char *cport = (char *)&(address.port);
	char buffer[] = { 'V', 'C', 'M', 'P', cip[0], cip[1], cip[2], cip[3], cport[0], cport[1], opcode };

	sendto(g_UDPSocket, buffer, sizeof(buffer), 0, (sockaddr *)&sendaddr, sizeof(sendaddr));
}
