

import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the Pintos process is terminated
def force_pintos_termination(pintos_process):
	c.close(force=True)

definitions_scriptname = sys.argv[1]
def_module = imp.load_source('', definitions_scriptname)
logfile = None
if hasattr(def_module, 'logfile'):
	logfile = def_module.logfile

kernel = sys.argv[2]
kernel_location = def_module.kernel[kernel]
os.chdir(kernel_location)

c = pexpect.spawn(def_module.shell, drainpty=True, logfile=logfile)
atexit.register(force_pintos_termination, pintos_process=c)

c.timeout = 10

# Give Pintos time to boot
time.sleep(1)

assert c.expect('Username:') == 0, "Shell did not ask for username"
c.sendline('root')

assert c.expect('Password:') == 0, "Shell did not ask for password"
c.sendline('root')

shellio.success()
