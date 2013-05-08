import sys, imp, atexit, filecmp
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the Pintos process is terminated
def force_pintos_termination(pintos_process):
	c.close(force=True)

definitions_scriptname = sys.argv[1]
def_module = imp.load_source('', definitions_scriptname)
logfile = file("log/sudo_test.log", "w")

kernel = sys.argv[2]
kernel_location = def_module.kernel[kernel]
cur_dir = os.getcwd()
os.chdir(kernel_location)

c = pexpect.spawn(def_module.login, drainpty=True, logfile=logfile)
atexit.register(force_pintos_termination, pintos_process=c)

c.timeout = def_module.pintos_timeout

# Give Pintos time to boot
time.sleep(def_module.pintos_bootup)

assert c.expect('Username:') == 0, "Login did not ask for username"
c.sendline('kevin')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.sendline('kevin')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.sendline('cat etc/group')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.sendline('sudo cat etc/group')

name = shellio.parse_regular_expression(c, '\n\[Sudo] password for (\w+):')
assert str(name[0]) == 'kevin', "sudo did not ask for password correctly"
c.sendline('kevin')

assert c.expect('root:x:0:root') == 0
assert c.expect('sudo:x:27:kevin mike ollie') == 0
#code = shellio.parse_regular_expression(c, 'cat: exit\((\d)\)')
#assert code[0] == 0
#code2 = shellio.parse_regular_expression(c, 'sudo: exit\((\d)\)')
#assert code2[0] == 0, str(code2)

c.expect('"sudo cat etc/group": exit code 0') == 0

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('exit\r')

shellio.success()
