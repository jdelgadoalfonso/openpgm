# -*- mode: python -*-
# OpenPGM build script

import platform
import os
import time
import sys

EnsureSConsVersion( 1, 0 )
SConsignFile('scons.signatures' + '-' + platform.system() + '-' + platform.machine());

vars = Variables()
vars.AddVariables (
	EnumVariable ('BUILD', 'build environment', 'debug',
			allowed_values=('release', 'debug', 'profile')),
	EnumVariable ('BRANCH', 'branch prediction', 'none',
			allowed_values=('none', 'profile', 'seed')),
	EnumVariable ('WITH_GETTEXT', 'l10n support via libintl', 'false',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_GLIB', 'Build GLib dependent modules', 'false',
			allowed_values=('true', 'false')),
	EnumVariable ('COVERAGE', 'test coverage', 'none',
			allowed_values=('none', 'full')),
	EnumVariable ('WITH_HISTOGRAMS', 'Runtime statistical information', 'true',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_HTTP', 'HTTP administration', 'false',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_SNMP', 'SNMP administration', 'false',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_CHECK', 'Check test system', 'false',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_TEST', 'Network test system', 'false',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_CC', 'C++ examples', 'true',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_EXAMPLES', 'Examples', 'true',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_NCURSES', 'NCURSES examples', 'false',
			allowed_values=('true', 'false')),
	EnumVariable ('WITH_PROTOBUF', 'Google Protocol Buffer examples', 'false',
			allowed_values=('true', 'false')),
)

#-----------------------------------------------------------------------------
# Platform specifics

env = Environment(
	variables = vars,
	ENV = os.environ,
	CCFLAGS = [	'-pipe',
			'-Wall',
				'-Wextra',
				'-Wfloat-equal',
				'-Wshadow',
				'-Wunsafe-loop-optimizations',
				'-Wpointer-arith',
				'-Wbad-function-cast',
				'-Wcast-qual',
				'-Wcast-align',
				'-Wwrite-strings',
				'-Waggregate-return',
				'-Wstrict-prototypes',
				'-Wold-style-definition',
				'-Wmissing-prototypes',
				'-Wmissing-declarations',
				'-Wmissing-noreturn',
				'-Wmissing-format-attribute',
				'-Wredundant-decls',
				'-Wnested-externs',
#				'-Winline',
				'-Wno-inline',
				'-Wno-unused-function',
			'-pedantic',
# C99
			'-std=gnu99',
#			'-D_XOPEN_SOURCE=600',
#			'-D_BSD_SOURCE',
# re-entrant libc
			'-D_REENTRANT',
# optimium checksum implementation
#			'-DUSE_8BIT_CHECKSUM',
			'-DUSE_16BIT_CHECKSUM',
#			'-DUSE_32BIT_CHECKSUM',
#			'-DUSE_64BIT_CHECKSUM',
#			'-DUSE_VECTOR_CHECKSUM',
# optimum galois field multiplication
			'-DUSE_GALOIS_MUL_LUT',
# Autoconf config.h
			'-DHAVE_CONFIG_H',
		],
	LINKFLAGS = [	'-pipe'
		],
	LIBS = [
		],
	PROTOBUF_CCFLAGS = '-I/miru/projects/protobuf/protobuf-2.3.0/gcc64/include',
	PROTOBUF_LIBS = '/miru/projects/protobuf/protobuf-2.3.0/gcc64/lib/libprotobuf.a',
	PROTOBUF_PROTOC = '/miru/projects/protobuf/protobuf-2.3.0/gcc64/bin/protoc'
)

# Branch prediction
if env['BRANCH'] == 'profile':
	env.Append(CCFLAGS = '-fprofile-arcs')
	env.Append(LINKFLAGS = '-fprofile-arcs')
elif env['BRANCH'] == 'seed':
	env.Append(CCFLAGS = '-fbranch-probabilities')

# Coverage analysis
if env['COVERAGE'] == 'full':
	env.Append(CCFLAGS = '-fprofile-arcs')
	env.Append(CCFLAGS = '-ftest-coverage')
	env.Append(LINKFLAGS = '-fprofile-arcs')
	env.Append(LINKFLAGS = '-lgcov')

# Define separate build environments
release = env.Clone(BUILD = 'release')
release.Append(CCFLAGS = '-O2')

debug = env.Clone(BUILD = 'debug')
debug.Append(CCFLAGS = ['-DPGM_DEBUG','-ggdb'], LINKFLAGS = '-gdb')

profile = env.Clone(BUILD = 'profile')
profile.Append(CCFLAGS = ['-O2','-pg'], LINKFLAGS = '-pg')

thirtytwo = release.Clone(BUILD = 'thirtytwo')
thirtytwo.Append(CCFLAGS = '-m32', LINKFLAGS = '-m32')

# choose and environment to build
if env['BUILD'] == 'release':
	Export({'env':release})
elif env['BUILD'] == 'profile':
	Export({'env':profile})
elif env['BUILD'] == 'thirtytwo':
	Export({'env':thirtytwo})
else:
	Export({'env':debug})

#-----------------------------------------------------------------------------
# Re-analyse dependencies

Import('env')

# vanilla environment
if env['WITH_GLIB'] == 'true':
	env['GLIB_FLAGS'] = env.ParseFlags('!pkg-config --cflags --libs glib-2.0 gthread-2.0');
else:
	env['GLIB_FLAGS'] = '';

# l10n
if env['WITH_GETTEXT'] == 'true':
	env.Append(CCFLAGS = '-DHAVE_GETTEXT');

# instrumentation
if env['WITH_HTTP'] == 'true' and env['WITH_HISTOGRAMS'] == 'true':
	env.Append(CCFLAGS = '-DUSE_HISTOGRAMS');

# managed environment for libpgmsnmp, libpgmhttp
if env['WITH_SNMP'] == 'true':
	env['SNMP_FLAGS'] = env.ParseFlags('!net-snmp-config --agent-libs');

def CheckSNMP(context):
	context.Message('Checking Net-SNMP...');
#	backup = context.env.Clone().Dictionary();
	lastASFLAGS	= context.env.get('ASFLAGS', '');
	lastCCFLAGS	= context.env.get('CCFLAGS', '');
	lastCFLAGS	= context.env.get('CFLAGS', '');
	lastCPPDEFINES	= context.env.get('CPPDEFINES', '');
	lastCPPFLAGS	= context.env.get('CPPFLAGS', '');
	lastCPPPATH	= context.env.get('CPPPATH', '');
	lastLIBPATH	= context.env.get('LIBPATH', '');
	lastLIBS	= context.env.get('LIBS', '');
	lastLINKFLAGS	= context.env.get('LINKFLAGS', '');
	lastRPATH	= context.env.get('RPATH', '');
	context.env.MergeFlags(env['SNMP_FLAGS']);
	result = context.TryLink("""
int main(int argc, char**argv)
{
	init_agent("PGM");
	return 0;
}
""", '.c');
#	context.env.Replace(**backup);
	context.env.Replace(ASFLAGS	= lastASFLAGS,
			    CCFLAGS	= lastCCFLAGS,
			    CFLAGS	= lastCFLAGS,
			    CPPDEFINES	= lastCPPDEFINES,
			    CPPFLAGS	= lastCPPFLAGS,
			    CPPPATH	= lastCPPPATH,
			    LIBPATH	= lastLIBPATH,
			    LIBS	= lastLIBS,
			    LINKFLAGS	= lastLINKFLAGS,
			    RPATH	= lastRPATH);
	context.Result(not result);
	return result;

def CheckCheck(context):
	context.Message('Checking Check unit test framework...');
	result = context.TryAction('pkg-config --cflags --libs check')[0];
	context.Result(result);
	return result;

tests = {
	'CheckCheck':	CheckCheck,
}
if env['WITH_SNMP'] == 'true':
	tests['CheckSNMP'] = CheckSNMP;
conf = Configure(env, custom_tests = tests);

if env['WITH_SNMP'] == 'true' and not conf.CheckSNMP():
	print 'Net-SNMP libraries not compatible.';
	Exit(1);

if env['WITH_CHECK'] == 'true' and conf.CheckCheck():
	print 'Enabling Check unit tests.';
	conf.env['CHECK'] = 'true';
	env['CHECK_FLAGS'] = env.ParseFlags('!pkg-config --cflags --libs check');
else:
	print 'Disabling Check unit tests.';
	conf.env['CHECK'] = 'false';

env = conf.Finish();

# add builder to create PIC static libraries for including in shared libraries
action_list = [ Action("$ARCOM", "$ARCOMSTR") ];
if env.Detect('ranlib'):
	ranlib_action = Action("$RANLIBCOM", "$RANLIBCOMSTR");
	action_list.append(ranlib_action);
pic_lib = Builder(	action = action_list,
			emitter = '$LIBEMITTER',
			prefix = '$LIBPREFIX',
			suffix = '$LIBSUFFIX',
			src_suffix = '$OBJSUFFIX',
			src_builder = 'SharedObject')
env.Append(BUILDERS = {'StaticSharedLibrary': pic_lib});


#-----------------------------------------------------------------------------

ref_node = 'ref/' + env['BUILD'] + '-' + platform.system() + '-' + platform.machine() + '/';
BuildDir(ref_node, '.', duplicate=0)

env.Append(CPPPATH = [
# $(top_builddir)/include
	os.getcwd() + '/' + ref_node + 'include',
# $(srcdir)/include
	os.getcwd() + '/include'
		]);
env.Append(LIBPATH = os.getcwd() + '/' + ref_node);

SConscript(ref_node + 'SConscript.autoconf');
if env['WITH_GLIB'] == 'true':
	SConscript(ref_node + 'SConscript.libpgmex');
SConscript(ref_node + 'SConscript.libpgm');
if env['WITH_HTTP'] == 'true':
	SConscript(ref_node + 'SConscript.libpgmhttp');
if env['WITH_SNMP'] == 'true':
	SConscript(ref_node + 'SConscript.libpgmsnmp');
if env['WITH_TEST'] == 'true':
	SConscript(ref_node + 'test/SConscript');
if env['WITH_EXAMPLES'] == 'true':
	SConscript(ref_node + 'examples/SConscript');

# end of file
