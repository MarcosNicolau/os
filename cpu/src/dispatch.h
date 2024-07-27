#ifndef DISTPATCH_H
#define DISPATCH_H

#include<context/context.h>
#include<proto/proto.h>

extern int cli_dispatch_fd;

void send_dispatch_reason(t_opcode reason,t_exec_context* contxt);
int wait_for_context(t_exec_context* contxt);

#endif