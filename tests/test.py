import sys
import datetime
# import cppimport
# funcs = cppimport.imp("../src/main.cpp")

import python_example as m

# assert m.__version__ == '0.0.1'
# assert m.add(1, 2) == 3
# assert m.subtract(1, 2) == -1

# f = open("foo.out", "w")
prof = m.Profiler("foo.out")
sys.setprofile(prof.tracer)



def bin():
  return 5

def bonzo():
  return 10

# bar = list('one')
# print(bar)
def bar(somearg):
  somearg += 1
  datetime.datetime.now()
  print("foo")
  print(datetime.datetime.now())
  (lambda foo: foo + "1")("hello")
  return somearg * bin() * bonzo()


bar(5)
# class Foo(object):
#   def catter(self):
#     return 'fooC'


# m.tracer2(Foo(), "foo2")
