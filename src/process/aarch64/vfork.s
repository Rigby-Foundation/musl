.global vfork
.type vfork,%function
vfork:
	mov x8, 3      // sic SYS_vfork
	svc 0
	.hidden __syscall_ret
	b __syscall_ret
