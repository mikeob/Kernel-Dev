

import sys, imp, atexit
sys.path.append("/home/courses/cs3214/software/pexpect-dpty/");
import pexpect, shellio, signal, time, os, re, proc_check

#Ensure the Pintos process is terminated
def force_pintos_termination(pintos_process):
	c.close(force=True)

definitions_scriptname = sys.argv[1]
def_module = imp.load_source('', definitions_scriptname)
logfile = file("log/passwd_test.log", "w")

kernel = sys.argv[2]
kernel_location = def_module.kernel[kernel]
os.chdir(kernel_location)

c = pexpect.spawn(def_module.getty, drainpty=True, logfile=logfile)
atexit.register(force_pintos_termination, pintos_process=c)

c.timeout = 30

# Give Pintos time to boot
time.sleep(7)

assert c.expect('Username:') == 0, "Login did not ask for username"
c.send('ollie\r')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.send('ollie\r')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('passwd\r')

assert c.expect('Old password:') == 0, "Passwd did not ask for old password"
c.send('mike\r')

assert c.expect('Incorrect password.') == 0, "Passwd accepted incorrect password"

assert c.expect('Old password:') == 0, "Passwd did not ask for old password"
c.send('ollie\r')

assert c.expect('New password:') == 0, "Passwd did not ask for new password"
c.send('new\r')

assert c.expect('Re-enter password:') == 0, "Passwd did not confirm new password"
c.send('ollie\r')

assert c.expect('Passwords do not match. Please try again.') == 0, "Passwd accepted non matching passwords"

assert c.expect('New password:') == 0, "Passwd did not ask for new password"
c.send('new\r')

assert c.expect('Re-enter password:') == 0, "Passwd did not confirm new password"
c.send('new\r')

assert c.expect('Password changed.') == 0, "Passwd change was not confirmed"
c.send('exit\r')
time.sleep(5)

assert c.expect('Username:') == 0, "Login did not ask for username"
c.send('ollie\r')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.send('ollie\r')

assert c.expect('Login failed.') == 0, "Login accepted old password"

assert c.expect('Username:') == 0, "Login did not ask for username"
c.send('ollie\r')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.send('new\r')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('exit\r')
time.sleep(5)

assert c.expect('Username:') == 0, "Login did not ask for username"
c.send('root\r')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.send('root\r')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('passwd\r')

assert c.expect('Username:') == 0, "Passwd did not ask root for username"
c.send('ollie\r')

assert c.expect('New password:') == 0, "Passwd did not ask for new password"
c.send('newroot\r')

assert c.expect('Re-enter password:') == 0, "Passwd did not confirm new password"
c.send('newroot\r')

assert c.expect('Password changed.') == 0, "Passwd change was not confirmed"
c.send('exit\r')
time.sleep(5)

assert c.expect('Username:') == 0, "Login did not ask for username"
c.send('ollie\r')

assert c.expect('Password:') == 0, "Login did not ask for password"
c.send('newroot\r')

assert c.expect(def_module.prompt) == 0, "Shell did not start"
c.send('exit\r')
time.sleep(5)

shellio.success()
