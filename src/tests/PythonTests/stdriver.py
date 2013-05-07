import getopt, os, sys, subprocess, re

# add directory in which script is located to python path
script_dir = "/".join(__file__.split("/")[:-1])
if script_dir == "":
	script_dir = "."
if script_dir not in sys.path:
	sys.path.append(script_dir)

verbose = False
output_spec_file = "./shelloutput.py"
test_base = ""
basic_test_file = "basic.tst"
testfilter = None
kernel = None

def usage():
	print """
Usage: python stdriver.py [options] [tests1.tst tests2.tst] ...
	-k project			userprog, threads, vm
	-t testname			Run only tests who names contains 'testname'
	-b 							Run basic tests
	-v 							Verbose
	"""

try:
	opts, args = getopt.getopt(sys.argv[1:], "t:bvk:", \
		["help", "test="])
except getopt.Getopterror, err:
	print str(err)
	usage()
	sys.exit(2)

for o, a in opts:
	if o == "-v":
		verbose = True
	elif o in ("-t"):
		testfilter = a;
	elif o in ("-b"):
		args = [test_base + basic_test_file]
	elif o in ("-k"):
		kernel = a;
	else:
		assert False, "unhandled option"

if not os.access(output_spec_file, os.R_OK):
		print "File ", output_spec_file, " is not readable"
		sys.exit(1)

full_testlist = []
if len(args) == 0:
	usage()
	sys.exit(1)

for testlist_filename in args:
	try:
		testlist_file = open(testlist_filename, 'r')
		test_dir = os.path.dirname(testlist_filename)
		if test_dir == "":
			test_dir = "./"
		else:
			test_dir = test_dir + '/'
	except:
		print 'Error: Tests list file name ''%s'' could not be opened. '% testlist_filename
		sys.exit(-1)

	# File input, read in the test filenames
	for line in testlist_file:
		grps = re.match("^= (.+)", line)
		if grps:
			testset = { 'name' : grps.group(1),
                  'tests' : [ ],
                  'dir' : test_dir }
			full_testlist.append(testset)
		else:
			grps = re.match("(\d+) (.+)", line)
			if grps:
				points, testname = int(grps.group(1)), grps.group(2)
				if not testfilter or testname.find(testfilter) != -1:
					testset['tests'].append((points, testname))
	testlist_file.close()

# print full_testlist

process_list = []

#Run through each test set in the list
for testset in full_testlist:
	print testset['name']
	print '-------------------------'

  #Run through each test in the set
	testresults = [ ]
	for (points, testname) in testset['tests']:
		print str(points) + '\t' + testname + ':',
		sys.stdout.flush()

    # run test
		child_process = subprocess.Popen(["python", testset['dir'] + testname, \
    	output_spec_file, kernel],\
			stderr=subprocess.PIPE, stdout=subprocess.PIPE)
		test_passed = child_process.wait() == 0

    # build result list
		testresults.append((points, testname, child_process, test_passed))

		#if: test passed
		if test_passed:
			print ' PASS'
		else:
			print ' FAIL'
		print ""
		testset['tests'] = testresults

    #Grade sheet printing.  It prints each individual test and the points awarded
    #from that test, the points awarded from each set of tests, and the total
    #points awarded over the entire test.

    #Run through each set of tests in the list
		testset_points = 0
		testset_points_earned = 0

    #Run through each test in the set
		for (points, testname, child_process, test_passed) in testset['tests']:
			testset_points += points
			points_earned = points
			if not test_passed:
				points_earned = 0
				
			print '\t%s\t%d/%d' % (testname, points_earned, points)
			testset_points_earned += points_earned
				
			print '-------------------------'
			
		print testset['name'] + '\t' + str(testset_points_earned) + '/' + \
   		str(testset_points)


#Verbose printing. If the verbose option was enabled, print the error
#information from the tests that failed.
if verbose:
	print '\nError Output'
	print '-------------------------'
	for testset in full_testlist:
		for (points, testname, child_process, test_passed) in testset['tests']:
			if not test_passed:
				print testname + ':'
				(stdout, stderr) = child_process.communicate()
				print stdout, stderr

