

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

c.timeout = 90

# Give Pintos time to boot
time.sleep(7)

assert c.expect('Username:') == 0, "Login did not ask for username"
c.sendline('kevin')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.sendline('kevin')

assert c.expect(def_module.prompt) == 0, "Shell prompt did not load"
c.sendline('cat etc/group')
time.sleep(3)

assert c.expect('exec failed\n'+def_module.prompt) == 0, "Error"
c.send('sudo cat')
time.sleep(3)
c.sendline(' etc/group')
time.sleep(5)

assert c.expect('[Sudo] password for kevin:') == 0, "Sudo did not prompt for password"
c.send('kevin\r')

assert c.expect('root:x:0:root\nsudo:x:27:kevin mike ollie \
	cat: exit(0)\nsudo: exit(0)\n"sudo cat etc/group": exit code 0') == 0, "did not cat etc/group"

c.send('exit\r')
time.sleep(5)
shellio.success()
