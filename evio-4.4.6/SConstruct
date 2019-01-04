################################
# scons build system file
################################
#
# Much of this file can be used as a boilerplate build file.
#
# The parts that are dependent on the details of the software
# package being compiled are:
#
#   1) the name and version of this software package
#   2) the needed header files and libraries
#   3) what type of documentation needs to be installed
#   4) the scons files to be run in the source directories
#
################################

# Get various python modules
import re, sys, os, string, subprocess, SCons.Node.FS
from subprocess import Popen, PIPE

# Get useful python functions we wrote
import coda

# Created files & dirs will have this permission
os.umask(002)

# Software version
versionMajor = '4'
versionMinor = '4.6'

# Determine the os and machine names
uname    = os.uname();
platform = uname[0]
machine  = uname[4]
osname   = os.getenv('CODA_OSNAME', platform + '-' +  machine)

# Create an environment while importing the user's PATH.
# This allows us to get to the vxworks compiler for example.
# So for vxworks, make sure the tools are in your PATH
env = Environment(ENV = {'PATH' : os.environ['PATH']})


################################
# 64 or 32 bit operating system?
################################
    
# How many bits is the operating system?
# For Linux 64 bit x86 machines, the "machine' variable is x86_64,
# but for Darwin or Solaris there is no obvious check so run
# a configure-type test.
is64bits = coda.is64BitMachine(env, platform, machine)
if is64bits:
    print "We're on a 64-bit machine"
else:
    print "We're on a 32-bit machine"


#############################################
# Command line options (view with "scons -h")
#############################################

Help('\n-D                  build from subdirectory of package\n')
Help('\nlocal scons OPTIONS:\n')

# debug option
AddOption('--dbg', dest='ddebug', default=False, action='store_true')
debug = GetOption('ddebug')
if debug: print "Enable debugging"
Help('--dbg               compile with debug flag\n')

# vxworks 5.5 option
AddOption('--vx5.5', dest='doVX55', default=False, action='store_true')
useVxworks55 = GetOption('doVX55')
if useVxworks55: print "Use vxWorks version 5.5"
Help('--vx5.5             cross compile for vxworks 5.5\n')

# vxworks 6.0 option
AddOption('--vx6.0', dest='doVX60', default=False, action='store_true')
useVxworks60 = GetOption('doVX60')
if useVxworks60: print "Use vxWorks version 6.0"
Help('--vx6.0             cross compile for vxworks 6.0\n')

# 32 bit option
AddOption('--32bits', dest='use32bits', default=False, action='store_true')
use32bits = GetOption('use32bits')
if use32bits: print "use 32-bit libs & executables even on 64 bit system"
Help('--32bits            compile 32bit libs & executables on 64bit system\n')

# install directory option
AddOption('--prefix', dest='prefix', nargs=1, default='', action='store')
prefix = GetOption('prefix')
Help('--prefix=<dir>      use base directory <dir> when doing install\n')

# include install directory option
AddOption('--incdir', dest='incdir', nargs=1, default=None, action='store')
incdir = GetOption('incdir')
Help('--incdir=<dir>      copy header  files to directory <dir> when doing install\n')

# library install directory option
AddOption('--libdir', dest='libdir', nargs=1, default=None, action='store')
libdir = GetOption('libdir')
Help('--libdir=<dir>      copy library files to directory <dir> when doing install\n')

# binary install directory option
AddOption('--bindir', dest='bindir', nargs=1, default=None, action='store')
bindir = GetOption('bindir')
Help('--bindir=<dir>      copy binary  files to directory <dir> when doing install\n')


#########################
# Compile flags
#########################

# 2 possible versions of vxWorks
useVxworks = False
if useVxworks55:
    useVxworks = True
    vxVersion  = 5.5
elif useVxworks60:
    useVxworks = True
    vxVersion  = 6.0


# Debug/optimization flags
debugSuffix = ''
if debug:
    debugSuffix = '-dbg'
# Compile with -g and add debugSuffix to all executable names
    env.Append(CCFLAGS = '-g', PROGSUFFIX = debugSuffix)

elif platform == 'SunOS':
    env.Append(CCFLAGS = '-xO3')

else:
    env.Append(CCFLAGS = '-O3')


# Take care of 64/32 bit issues
if is64bits:
    # Setup 64 bit machine to compile either 32 or 64 bit libs & executables
    coda.configure32bits(env, use32bits, platform)
elif not use32bits:
    use32bits = True


execLibs = ['']


# If using vxworks ...
if useVxworks:
    
    use32bits = True
    osname = 'vxworks'+ str(vxVersion) + '-ppc'

    if not coda.configureVxworks(env, vxVersion, platform):
        print '\nCannot set enviroment for vxWorks ' + str(vxVersion) + ', exiting\n'
        Exit(0)

else:
    # Platform dependent quantities. Default to standard Linux libs.
    execLibs = ['pthread', 'expat', 'z', 'dl', 'm']
    
    if platform == 'SunOS':
        execLibs = ['m', 'posix4', 'pthread', 'dl', 'expat', 'z']
        env.Append(CCFLAGS = ['-mt'])
        env.Append(CPPDEFINES = ['_GNU_SOURCE', '_REENTRANT', '_POSIX_PTHREAD_SEMANTICS', 'SunOS'])

    elif platform == 'Darwin':
        execLibs = ['pthread', 'dl', 'expat', 'z']
        env.Append(CPPDEFINES = ['Darwin'], SHLINKFLAGS = ['-multiply_defined','suppress','-undefined','dynamic_lookup'])
        env.Append(CCFLAGS = ['-fmessage-length=0'])



if is64bits and use32bits and not useVxworks:
    osname = osname + '-32'

print "OSNAME =", osname

# hidden sub directory into which variant builds go
archDir = '.' + osname + debugSuffix


#########################
# Install stuff
#########################

libInstallDir     = ""
binInstallDir     = ""
incInstallDir     = ""
archIncInstallDir = ""

# If a specific installation dir for includes is not specified
# on the command line, then the architecture specific include
# dir is a link to local ../common/include directory.
archIncLocalLink = None
if (incdir == None):
    archIncLocalLink = '../common/include'

# If we going to install ...
if 'install' in COMMAND_LINE_TARGETS:
    # Determine installation directories
    installDirs = coda.getInstallationDirs(osname, prefix, incdir, libdir, bindir)
    
    mainInstallDir    = installDirs[0]
    osDir             = installDirs[1]
    incInstallDir     = installDirs[2]
    archIncInstallDir = installDirs[3]
    libInstallDir     = installDirs[4]
    binInstallDir     = installDirs[5]
    
    # Create the include directories (make symbolic link if possible)
    coda.makeIncludeDirs(incInstallDir, archIncInstallDir, osDir, archIncLocalLink)

    print 'Main install dir  = ', mainInstallDir
    print 'bin  install dir  = ', binInstallDir
    print 'lib  install dir  = ', libInstallDir
    print 'inc  install dirs = ', incInstallDir, ", ", archIncInstallDir

else:
    print 'No installation being done'

print

# use "install" on command line to install libs & headers
Help('install             install libs, headers, and binaries\n')

# uninstall option
Help('install -c          uninstall libs, headers, and binaries\n')


###############################
# Documentation (un)generation
###############################

if 'doc' in COMMAND_LINE_TARGETS:
    coda.generateDocs(env, True, True, True, "java/org/jlab/coda/jevio")

if 'undoc' in COMMAND_LINE_TARGETS:
    coda.removeDocs(env)


# use "doc" on command line to create tar file
Help('doc                 create javadoc (in ./doc)\n')

# use "undoc" on command line to create tar file
Help('undoc               remove javadoc (in ./doc)\n')


#########################
# Tar file
#########################

if 'tar' in COMMAND_LINE_TARGETS:
    if platform == 'SunOS':
        print '\nMake tar file from Linux or MacOS please\n'
        Exit(0)
    coda.generateTarFile(env, 'evio', versionMajor, versionMinor)

# use "tar" on command line to create tar file
Help('tar                 create tar file (in ./tar)\n')


######################################################
# Lower level scons files (software package dependent)
######################################################

# Make available to lower level scons files
Export('env archDir incInstallDir libInstallDir binInstallDir archIncInstallDir execLibs debugSuffix')

# Run lower level build files
#if useVxworks:
#    env.SConscript('src/libsrc/SConscript.vx',   variant_dir='src/libsrc/'+archDir,   duplicate=0)
#    env.SConscript('src/libsrc++/SConscript.vx', variant_dir='src/libsrc++/'+archDir, duplicate=0)
#else:
env.SConscript('src/libsrc/SConscript',   variant_dir='src/libsrc/'+archDir,   duplicate=0)

if not useVxworks:
    env.SConscript('src/libsrc++/SConscript', variant_dir='src/libsrc++/'+archDir, duplicate=0)
    env.SConscript('src/execsrc/SConscript',  variant_dir='src/execsrc/'+archDir,  duplicate=0)
    env.SConscript('src/examples/SConscript', variant_dir='src/examples/'+archDir, duplicate=0)
    env.SConscript('src/test/SConscript',     variant_dir='src/test/'+archDir,     duplicate=0)
