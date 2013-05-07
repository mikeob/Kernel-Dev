"""Module docstring.

This serves as a long usage message.
"""
import sys
import getopt
import string

def main():
  file = None
  # parse command line options
  try:
    opts, args = getopt.getopt(sys.argv[1:], "h", ["help"])
  except getopt.error, msg:
    print msg
    print "for help use --help"
    sys.exit(2)
    # process options
  for o, a in opts:
    if o in ("-h", "--help"):
      print __doc__
      sys.exit(0)
    # process arguments
  for arg in args:
    filename = arg
    escaperegex(filename)

def escaperegex(filename):
  f = open(filename, "rw")
  new = open(string.replace(filename, "log", "cmp"), "w+")

  escapes = ['.', '[', '*', '(', ')']

  for line in f:
    for char in line:
      char_written = False
      for esc in escapes:
        if char == esc:
          new.write("\\" + char)
          char_written = True
          break
      if not char_written:
        new.write(char)
        char_written = False
  new.close()
  f.close()

if __name__ == "__main__":
  main()
