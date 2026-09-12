	nop
.global __restore_rt
.hidden __restore_rt
.type __restore_rt,@function
__restore_rt:
	mov $162, %rax        /* sic SYS_rt_sigreturn */
	syscall
.size __restore_rt,.-__restore_rt
