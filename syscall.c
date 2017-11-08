#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <machine/pcb.h>
#include <machine/spl.h>
#include <machine/trapframe.h>
#include <kern/callno.h>
#include <syscall.h>


/*
 * System call handler.
 *
 * A pointer to the trapframe created during exception entry (in
 * exception.S) is passed in.
 *
 * The calling conventions for syscalls are as follows: Like ordinary
 * function calls, the first 4 32-bit arguments are passed in the 4
 * argument registers a0-a3. In addition, the system call number is
 * passed in the v0 register.
 *
 * On successful return, the return value is passed back in the v0
 * register, like an ordinary function call, and the a3 register is
 * also set to 0 to indicate success.
 *
 * On an error return, the error code is passed back in the v0
 * register, and the a3 register is set to 1 to indicate failure.
 * (Userlevel code takes care of storing the error code in errno and
 * returning the value -1 from the actual userlevel syscall function.
 * See src/lib/libc/syscalls.S and related files.)
 *
 * Upon syscall return the program counter stored in the trapframe
 * must be incremented by one instruction; otherwise the exception
 * return code will restart the "syscall" instruction and the system
 * call will repeat forever.
 *
 * Since none of the OS/161 system calls have more than 4 arguments,
 * there should be no need to fetch additional arguments from the
 * user-level stack.
 *
 * Watch out: if you make system calls that have 64-bit quantities as
 * arguments, they will get passed in pairs of registers, and not
 * necessarily in the way you expect. We recommend you don't do it.
 * (In fact, we recommend you don't use 64-bit quantities at all. See
 * arch/mips/include/types.h.)
 */

void
mips_syscall(struct trapframe *tf)
{
	int callno;
	int32_t retval;
	int err;

	assert(curspl==0);

	callno = tf->tf_v0;

	/*
	 * Initialize retval to 0. Many of the system calls don't
	 * really return a value, just 0 for success and -1 on
	 * error. Since retval is the value returned on success,
	 * initialize it to 0 by default; thus it's not necessary to
	 * deal with it except for calls that return other values,
	 * like write.
	 */

	retval = 0;

	switch (callno) {
	    case SYS_reboot:
		err = sys_reboot(tf->tf_a0);
		break;

	    /* Add stuff here */

	    default:
		kprintf("Unknown syscall %d\n", callno);
		err = ENOSYS;
		break;
	}


	if (err) {
		/*
		 * Return the error code. This gets converted at
		 * userlevel to a return value of -1 and the error
		 * code in errno.
		 */
		tf->tf_v0 = err;
		tf->tf_a3 = 1;      /* signal an error */
	}
	else {
		/* Success. */
		tf->tf_v0 = retval;
		tf->tf_a3 = 0;      /* signal no error */
	}

	/*
	 * Now, advance the program counter, to avoid restarting
	 * the syscall over and over again.
	 */

	tf->tf_epc += 4;

	/* Make sure the syscall code didn't forget to lower spl */
	assert(curspl==0);
}

int execv(const char*program, char **args){
#define MAX_PATH_LENGTH 128
/*
execv replaces the currently executing program with a newly loaded program image.

This occurs within one process; the process id is unchanged.
The pathname of the program to run is passed as program.
The args argument is an array of 0-terminated strings.
The array itself should be terminated by a NULL pointer.

The argument strings should be copied into the new process as the new process's argv[] array.

In the new process, argv[argc] must be NULL.

By convention, argv[0] in new processes contains the name that was used to invoke the program.

This is not necessarily the same as program,

and furthermore is only a convention and should not be enforced by the kernel.

The process file table and current working directory are not modified by execve.
*/
  int s=splhigh();//disable all interrupts;

	if(program == NULL)
	     return EFAULT;//program doesnot exist, it is a invalid arguments;
 struct vnode *vn;
	int dir;
	dir=vfs_lookup(program,&vn);

	if(dir==0){
		return EISDIR;
	}

	char* programSize=(char*)kmalloc(MAX_PATH_LENGTH*sizeof(char));

	struct vnode*v;
	vaddr_t entrypoint0,stackptr;
	int result;

	result=vfs_open(progname,O_RDONLY,&v);// open the file
	if(result){
		return result;
	}

	if (curthread->t_vmspace=!NULL){
		as_destroy(curthread->t_vmspace);
		curthread->t_vmspace=NULL;
		curthread->t_vmspace=as_create();
	}// once we be a new thread
	else{
		vfs_close(v);
		return ENOMEN;
	}

	as_activate(curthread->t_vmspace);
	result=load_elf(v,&entrypoint);
	if(result){
		vfs_close(v);
		return result;
	}

	vfs_close(v);


result=as_define_stack(curthread->t_vmspace,&stackptr);
if(result){
	return result;
}

//copy the values to stack;
for(i=argc-1;i>=0;i--){
	stackptr-=4;
	result=copyout(pointers+i,stackptr,4);
}

md_usermode(0,NULL,stackptr,entrypoint);

panic("md_usermode returned\n");
return EINVAL;
}



void
md_forkentry(struct trapframe *tf)
{
	/*
	 * This function is provided as a reminder. You need to write
	 * both it and the code that calls it.
	 *
	 * Thus, you can trash it and do things another way if you prefer.
	 */

	(void)tf;
}
