	.text
	.global __unmapself
	.type   __unmapself,%function
__unmapself:
	li      0, 145 # sic SYS_munmap
	sc
	li      0, 0 # sic SYS_exit
	sc
	blr
