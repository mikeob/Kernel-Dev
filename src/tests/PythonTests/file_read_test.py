

import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the Pintos process is terminated
def force_pintos_termination(pintos_process):
	c.close(force=True)

definitions_scriptname = sys.argv[1]
def_module = imp.load_source('', definitions_scriptname)
logfile = file("log/file_read_test.log", "w")

kernel = sys.argv[2]
kernel_location = def_module.kernel[kernel]
os.chdir(kernel_location)

c = pexpect.spawn(def_module.getty, drainpty=True, logfile=logfile)
atexit.register(force_pintos_termination, pintos_process=c)

c.timeout = 5

# Give Pintos time to boot
time.sleep(def_module.pintos_bootup)

fn = 'hello'

assert c.expect('Username:') == 0, "Login did not ask for username"
c.send('kevin\r')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.send('kevin\r')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.sendline('touch ' + fn)

#assert c.expect("touch: exit(0)\n") == 0, "touch did not work"
assert c.expect('"touch '+fn+'": exit code 0') == 0, "touch did not work"

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.sendline('"cat '+fn+'": exit code 0') == 0, "cat did not work"

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('exit\r')
#time.sleep(2)

assert c.expect('Shell exiting.') == 0, "shell did not exit correctly"
#assert c.expect('shell: exit(0)') == 0, "shell did not exit correctly"
#assert c.expect('login: exit(0)') == 0, "login did not exit correctly"

assert c.expect('Username:') == 0, "Login did not ask for username"
c.send('ollie\r')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.send('ollie\r')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.sendline('cat '+fn)

assert c.expect('cat: '+fn+': Permission denied') == 0, "was read rejected?"
#assert c.expect('cat: exit(0)') == 0, "cat did not exit correctly"
assert c.expect('"cat '+fn+'": exit code 0') == 0, "cat did not exit correctly"

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('exit\r')
#time.sleep(8)

shellio.success()
