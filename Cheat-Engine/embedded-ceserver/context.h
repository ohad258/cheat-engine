/*
 * context.h
 *
 *  Created on: May 18, 2015
 *      Author: eric
 */

#ifndef CONTEXT_H_
#define CONTEXT_H_

#ifdef HAS_LINUX_USER_H
#else
#endif


#ifdef __aarch64__
#include <elf.h>
#endif





#ifdef __i386__
  typedef struct pt_regs CONTEXT_REGS;
#endif

#ifdef __x86_64__
  typedef struct user_regs_struct CONTEXT_REGS;
#endif


#ifdef __arm__
  typedef struct pt_regs CONTEXT_REGS;
#endif


#ifdef __aarch64__
  typedef struct user_pt_regs CONTEXT_REGS;
#endif

#ifdef __mips__
/* TODO: rewrite all of this definitions */
  struct user_pt_regs
  {
    long regs[18];
  };

  struct user_hwdebug_state {
   uint32_t dbg_info;
   struct {
   uint32_t addr;
   uint32_t ctrl;
   } dbg_regs[16];
  };

#define NT_ARM_HW_WATCH 0x403
#define PTRACE_GETREGSET 0x4204
#define PTRACE_SETREGSET 0x4205
#endif

  /* TODO: make correct regs */
#ifdef __mips__
  typedef struct user_pt_regs CONTEXT_REGS;
#endif

typedef struct
{
    /* TODO: not real */
    uint32_t i;
//  CONTEXT_REGS regs;
} CONTEXT, *PCONTEXT;


int getRegisters(int tid, CONTEXT_REGS *registerstore);

#endif /* CONTEXT_H_ */
