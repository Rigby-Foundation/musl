.global __unmapself
.type   __unmapself,%function
__unmapself:
	mov x8,#145 // sic SYS_munmap
	svc 0
	mov x8,#0 // sic SYS_exit
	svc 0
