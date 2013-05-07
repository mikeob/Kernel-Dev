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

c.timeout = 30

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

time.sleep(2)

#assert c.expect("[Sudo] password for kevin: ") == 0, "sudo did not prompt for password"
c.sendline('kevin')

time.sleep(2) # Give time for output to print

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('exit\r')
time.sleep(5)

os.chdir(cur_dir)
log = file("log/sudo_test.log", "r")
log_text = log.read()
log.close()

cmp = file("cmp/sudo_test.cmp", "r")
cmp_text = cmp.read()
cmp.close()

assert re.search(cmp_text, log_text) != None, "output of operations is not correct"
shellio.success()
