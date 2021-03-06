###########################################################################
#
#    #####          #######         #######         ######            ###
#   #     #            #            #     #         #     #           ###
#   #                  #            #     #         #     #           ###
#    #####             #            #     #         ######             #
#         #            #            #     #         #
#   #     #            #            #     #         #                 ###
#    #####             #            #######         #                 ###
#
#
# Please read the directions in README and in this config.mk carefully.
# Do -N-O-T- just dump things randomly in here until your kernel builds.
# If you do that, you run an excellent chance of turning in something
# which can't be graded.  If you think the build infrastructure is
# somehow restricting you from doing something you need to do, contact
# the course staff--don't just hit it with a hammer and move on.
#
# [Once you've read this message, please edit it out of your config.mk]
# [Once you've read this message, please edit it out of your config.mk]
# [Once you've read this message, please edit it out of your config.mk]
###########################################################################

###########################################################################
# This is the include file for the make file.
# You should have to edit only this file to get things to build.
###########################################################################

###########################################################################
# Tab stops
###########################################################################
# If you use tabstops set to something other than the international
# standard of eight characters, this is your opportunity to inform
# our print scripts.
TABSTOP = 8

###########################################################################
# The method for acquiring project updates.
###########################################################################
# This should be "afs" for any Andrew machine, "web" for non-andrew machines
# and "offline" for machines with no network access.
#
# "offline" is strongly not recommended as you may miss important project
# updates.
#
UPDATE_METHOD = afs

###########################################################################
# WARNING: When we test your code, the two TESTS variables below will be
# blanked.  Your kernel MUST BOOT AND RUN if 410TESTS and STUDENTTESTS
# are blank.  It would be wise for you to test that this works.
###########################################################################

###########################################################################
# Test programs provided by course staff you wish to run
###########################################################################
# A list of the test programs you want compiled in from the 410user/progs
# directory.
#
# 410TESTS = new_pages remove_pages_test1 remove_pages_test2
# 410TESTS += wait_getpid fork_wait fork_wait_bomb fork_exit_bomb stack_test1 halt_test
# 410TESTS += swexn_basic_test swexn_cookie_monster swexn_regs swexn_dispatch
# 410TESTS += make_crash make_crash_helper sleep_test1 mem_permissions cho cho2
# 410TESTS += wild_test1 exec_basic print_basic exec_basic_helper
# 410TESTS += actual_wait yield_desc_mkrun
# 410TESTS += getpid_test1 loader_test1
# 410TESTS += cho_variant loader_test2

410TESTS = deschedule_hang exec_basic exec_basic_helper fork_test1 fork_wait
410TESTS += getpid_test1 halt_test loader_test1 mem_eat_test print_basic
410TESTS += readline_basic remove_pages_test1 sleep_test1 stack_test1
410TESTS += wait_getpid wild_test1 yield_desc_mkrun
410TESTS += exec_nonexist fork_bomb fork_exit_bomb fork_wait_bomb loader_test2
410TESTS += make_crash make_crash_helper mem_permissions minclone_mem new_pages
410TESTS += register_test remove_pages_test2
410TESTS += cho cho2 cho_variant
410TESTS += bg new_shell

###########################################################################
# Test programs you have written which you wish to run
###########################################################################
# A list of the test programs you want compiled in from the user/progs
# directory.
#
STUDENTTESTS = agility_drill cyclone join_specific_test rwlock_downgrade_read_test switzerland thr_exit_join
STUDENTTESTS += misbehave racer

###########################################################################
# Data files provided by course staff to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the 410user/files
# directory.
#
410FILES =

###########################################################################
# Data files you have created which you wish to build into the RAM disk
###########################################################################
# A list of the data files you want built in from the user/files
# directory.
#
STUDENTFILES =

###########################################################################
# Object files for your thread library
###########################################################################
THREAD_OBJS = malloc.o panic.o mutex.o condvar.o thread_create.o thread.o rwlock.o sem.o

# Thread Group Library Support.
#
# Since libthrgrp.a depends on your thread library, the "buildable blank
# P3" we give you can't build libthrgrp.a.  Once you install your thread
# library and fix THREAD_OBJS above, uncomment this line to enable building
# libthrgrp.a:
410USER_LIBS_EARLY += libthrgrp.a

###########################################################################
# Object files for your syscall wrappers
###########################################################################
SYSCALL_OBJS = syscall_wrapper.o

###########################################################################
# Object files for your automatic stack handling
###########################################################################
AUTOSTACK_OBJS = autostack.o

###########################################################################
# Parts of your kernel
###########################################################################
#
# Kernel object files you want included from 410kern/
#
410KERNEL_OBJS = load_helper.o
#
# Kernel object files you provide in from kern/
#
KERNEL_OBJS = kernel.o loader.o malloc_wrappers.o cpu.o
KERNEL_OBJS += driver.o graphic_driver.o int_handler.o keyboard_driver.o timer_driver.o
KERNEL_OBJS += vm.o pm.o zeus.o mode_switch.o process.o
KERNEL_OBJS += syscall.o syscall_handler.o syscall_lifecycle.o syscall_memory.o
KERNEL_OBJS += scheduler.o context_switch.o context_switch_c.o scheduler.o vm_asm.o
KERNEL_OBJS += source_untrusted.o
KERNEL_OBJS += kmutex.o
KERNEL_OBJS += keyboard_event.o syscall_consoleio.o syscall_fileio.o
KERNEL_OBJS += reaper.o
KERNEL_OBJS += fault.o fault_handler.o asm_wrapper.o
KERNEL_OBJS += mode_switch_c.o fault_handler_user.o syscall_thread.o
KERNEL_OBJS += timeout.o syscall_debug.o
KERNEL_OBJS += kernel_stack_protection.o
KERNEL_OBJS += hv.o hvseg.o hvlife.o
KERNEL_OBJS += hv_hpcall_s.o hv_hpcall.o hv_hpcall_misc.o hv_hpcall_consoleio.o
KERNEL_OBJS += hvinterrupt.o hvinterrupt_pushevent.o hv_hpcall_int.o
KERNEL_OBJS += virtual_console.o hv_hpcall_vm.o

###########################################################################
# WARNING: Do not put **test** programs into the REQPROGS variables.  Your
#          kernel will probably not build in the test harness and you will
#          lose points.
###########################################################################

###########################################################################
# Mandatory programs whose source is provided by course staff
###########################################################################
# A list of the programs in 410user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# The shell is a really good thing to keep here.  Don't delete idle
# or init unless you are writing your own, and don't do that unless
# you have a really good reason to do so.
#
410REQPROGS = idle init shell

###########################################################################
# Mandatory programs whose source is provided by you
###########################################################################
# A list of the programs in user/progs which are provided in source
# form and NECESSARY FOR THE KERNEL TO RUN.
#
# Leave this blank unless you are writing custom init/idle/shell programs
# (not generally recommended).  If you use STUDENTREQPROGS so you can
# temporarily run a special debugging version of init/idle/shell, you
# need to be very sure you blank STUDENTREQPROGS before turning your
# kernel in, or else your tweaked version will run and the test harness
# won't.
#
STUDENTREQPROGS =

CONFIG_DEBUG = user kernel

410GUESTBINS = hello dumper magic console station tick_tock gomoku cliff
410GUESTBINS += teeny vast warp fondle
410GUESTBINS += pathos
