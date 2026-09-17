	.global __restore
	.hidden __restore
	.type __restore,%function
__restore:
	li      0, 162 # sic SYS_rt_sigreturn (no legacy sigreturn)
	sc

	.global __restore_rt
	.hidden __restore_rt
	.type __restore_rt,%function
__restore_rt:
	li      0, 162 # sic SYS_rt_sigreturn
	sc
