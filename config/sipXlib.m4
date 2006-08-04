##
## Libs from SipFoundry
##
## Common C and C++ flags for pingtel related source
AC_DEFUN([SFAC_INIT_FLAGS],
[
    AC_SUBST(CPPUNIT_CFLAGS,  [])
    AC_SUBST(CPPUNIT_LDFLAGS, [])

    ## TODO Remove cpu specifics and use make variables setup for this
    ##
    ## NOTES:
    ##   -D__pingtel_on_posix__   - really used for linux v.s. other
    ##   -D_REENTRANT             - roguewave ?
    ##   -fmessage-length=0       - ?
    ##
    AC_SUBST(SIPX_INCDIR, [${includedir}])
    AC_SUBST(SIPX_LIBDIR, [${libdir}])
    AC_SUBST(SIPX_LIBEXECDIR, [${libexecdir}])

    CFLAGS="-I${prefix}/include $CFLAGS"
    CXXFLAGS="-I${prefix}/include $CXXFLAGS"
    LD_FLAGS="-L${prefix}/lib ${LDFLAGS}"

    if test x_"${ax_cv_c_compiler_vendor}" = x_gnu
    then
    	SF_CXX_C_FLAGS="-D__pingtel_on_posix__ -D_linux_ -D_REENTRANT -D_FILE_OFFSET_BITS=64 -fmessage-length=0"
    	SF_CXX_WARNINGS="-Wall -Wformat -Wwrite-strings -Wpointer-arith"
    	CXXFLAGS="$CXXFLAGS $SF_CXX_C_FLAGS $SF_CXX_WARNINGS"
    	CFLAGS="$CFLAGS $SF_CXX_C_FLAGS $SF_CXX_WARNINGS -Wnested-externs -Wmissing-declarations -Wmissing-prototypes"
     elif test x_"${ax_cv_c_compiler_vendor}" = x_sun
     then
	SF_CXX_C_FLAGS="-D__pingtel_on_posix__ -D_REENTRANT -D_FILE_OFFSET_BITS=64 -mt -fast -v"
	SF_CXX_FLAGS="-D__pingtel_on_posix__ -D_REENTRANT -D_FILE_OFFSET_BITS=64 -mt -xlang=c99 -fast -v"
	SF_CXX_WARNINGS=""
	CXXFLAGS="$CXXFLAGS $SF_CXX_FLAGS $SF_CXX_WARNINGS"
	CFLAGS="$CFLAGS $SF_CXX_C_FLAGS $SF_CXX_WARNINGS"
     else
        SF_CXX_C_FLAGS="-D__pingtel_on_posix__ -D_linux_ -D_REENTRANT -D_FILE_OFFSET_BITS=64"
        SF_CXX_WARNINGS=""
        CXXFLAGS="$CXXFLAGS $SF_CXX_C_FLAGS $SF_CXX_WARNINGS"
        CFLAGS="$CFLAGS $SF_CXX_C_FLAGS $SF_CXX_WARNINGS"
     fi

    ## set flag for gcc
    AM_CONDITIONAL(ISGCC, [test  x_"${GCC}" != x_])

    ## NOTE: These are not expanded (e.g. contain $(prefix)) and are only
    ## fit for Makefiles. You can however write a Makefile that transforms
    ## *.in to * with the concrete values. 
    ##
    ##  See sipXconfig/Makefile.am for an example.   
    ##  See autoconf manual 4.7.2 Installation Directory Variables for why it's restricted
    ##
    AC_SUBST(SIPX_BINDIR,  [${bindir}])
    AC_SUBST(SIPX_CONFDIR, [${sysconfdir}/sipxpbx])
    AC_SUBST(SIPX_DATADIR, [${datadir}/sipxpbx])
    AC_SUBST(SIPX_LOGDIR,  [${localstatedir}/log/sipxpbx])
    AC_SUBST(SIPX_RUNDIR,  [${localstatedir}/run/sipxpbx])
    AC_SUBST(SIPX_TMPDIR,  [${localstatedir}/sipxdata/tmp])
    AC_SUBST(SIPX_DBDIR,   [${localstatedir}/sipxdata/sipdb])
    AC_SUBST(SIPX_UPGRADEDIR,[${localstatedir}/sipxdata/upgrade])
    AC_SUBST(SIPX_SCHEMADIR, [${datadir}/sipx/schema])
    AC_SUBST(SIPX_DOCDIR,  [${datadir}/doc/sipx])
    AC_SUBST(SIPX_VARDIR,  [${localstatedir}/sipxdata])

    # temporary - see http://track.sipfoundry.org/browse/XPB-33
    AC_SUBST(SIPX_VXMLDATADIR,[${localstatedir}/sipxdata/mediaserver/data])

    AC_SUBST(SIPX_PARKMUSICDIR,[${localstatedir}/sipxdata/parkserver/music])

    # temporary - see http://track.sipfoundry.org/browse/XPB-93
    AC_SUBST(SIPX_BACKUPDIR, [${localstatedir}/sipxdata/backup])
    AC_SUBST(SIPX_CONFIGPHONEDIR, [${localstatedir}/sipxdata/configserver/phone])

    ## Used in a number of different project and subjective where this should really go
    ## INSTALL instruction assume default, otherwise safe to change/override
    AC_ARG_VAR(wwwdir, [Web root for web content, default is ${datadir}/www. \
                        WARNING: Adjust accordingly when following INSTALL instructions])
    test -z $wwwdir && wwwdir='${datadir}/www'

    AC_ARG_VAR(SIPXPBXUSER, [The sipX service daemon user name, default is 'sipx'])
    test -z $SIPXPBXUSER && SIPXPBXUSER=sipx

    AC_SUBST(SIPXPHONECONF, [${sysconfdir}/sipxphone])
    AC_SUBST(SIPXPHONEDATA, [${datadir}/sipxphone])
    AC_SUBST(SIPXPHONELIB, [${datadir}/sipxphone/lib])

    AC_ARG_ENABLE(rpmbuild, 
      [  --enable-rpmbuild       Build an rpm],
      enable_rpmbuild=yes )

    AC_ARG_ENABLE(buildnumber,
                 [  --enable-buildnumber    enable build number as part of RPM name],
                 enable_buildnumber=yes)

    # Enable profiling via gprof
    ENABLE_PROFILE

    SFAC_RPM_REPO
])


## Check to see that we are using the minimum required version of automake
AC_DEFUN([SFAC_AUTOMAKE_VERSION],[
   AC_MSG_CHECKING(for automake version >= $1)
   sf_am_version=`automake --version | head -n 1 | awk '/^automake/ {print $NF}'`
   AX_COMPARE_VERSION( [$1], [le], [$sf_am_version], AC_MSG_RESULT( $sf_am_version is ok), AC_MSG_ERROR( found $sf_am_version - you must upgrade automake ))
])

AC_DEFUN([SFAC_DISTRO_CONDITIONAL],
[
   distroid="${DISTRO}${DISTROVER}"
   AC_MSG_CHECKING(Distribution specific settings for '${distroid}')

   AM_CONDITIONAL([PLATFORM_FC4], [test "${distroid}" = "FC4"])
   AM_CONDITIONAL([PLATFORM_FC5], [test "${distroid}" = "FC5"])
   AM_CONDITIONAL([PLATFORM_RHE3],[test "${distroid}" = "RHE3"])
   AM_CONDITIONAL([PLATFORM_RHE4],[test "${distroid}" = "RHE4"])

   LIBWWW_RPM=w3c-libwww

   case "${DISTRO}${DISTROVER}" in
    FC4)
      AC_MSG_RESULT()
      ;;
    FC5)
      AC_MSG_RESULT()
      ;;
    RHE3)
      AC_MSG_RESULT([  using sipx version of libwww])
      LIBWWW_RPM=sipx-w3c-libwww
      ;;
    RHE4)
      AC_MSG_RESULT([  using sipx version of libwww])
      LIBWWW_RPM=sipx-w3c-libwww
      ;;
    *)
      AC_MSG_WARN(Unrecognized distribution '${DISTRO}${DISTROVER}')
      ;;
   esac

   AC_SUBST([LIBWWW_RPM])
])

## Soon to replace above macro...
AC_DEFUN([SFAC_DISTRO_CONDITIONAL2],
[
   AC_ARG_VAR(LIBWWW_RPM, [Name of package that support w3c-libwww support])
   if [ -z ${LIBWWW_RPM} ]
   then
       AC_MSG_CHECKING(Checking distribution version)
       RH_DISTRO = `cat /etc/redhat-release 2> /dev/null`
       case "$(RH_DISTRO)" in
         CentOS release 4* | \
         Red Hat Enterprise Linux ES release 4* | \
         Fedora Core release 3* )
           AC_MSG_RESULT([  using sipx version of libwww])
           LIBWWW_RPM = sipx-w3c-libwww
	   ;;
         *)
           AC_MSG_RESULT([  using distro supplied version of libwww])
           LIBWWW_RPM = w3c-libwww
           ;;
       esac
   fi
])

## sipXportLib 
# SFAC_LIB_PORT attempts to find the sf portability library and include
# files by looking in /usr/[lib|include], /usr/local/[lib|include], and
# relative paths.
#
# If not found, the configure is aborted.  Otherwise, variables are defined
# for both the INC and LIB paths 
# AND the paths are added to the CFLAGS and CXXFLAGS
AC_DEFUN([SFAC_LIB_PORT],
[
    AC_REQUIRE([SFAC_INIT_FLAGS])
    AC_REQUIRE([CHECK_PCRE])
    AC_REQUIRE([CHECK_SSL])
    AC_SUBST(SIPXPORT_LIBS, [-lsipXport])
    AC_SUBST(SIPXUNIT_LIBS, [-lsipXunit])
]) # SFAC_LIB_PORT


## sipXtackLib 
# SFAC_LIB_STACK attempts to find the sf networking library and include
# files by looking in /usr/[lib|include], /usr/local/[lib|include], and
# relative paths.
#
# If not found, the configure is aborted.  Otherwise, variables are defined
# for both the INC and LIB paths AND the paths are added to the CFLAGS, 
# CXXFLAGS, LDFLAGS, and LIBS.
AC_DEFUN([SFAC_LIB_STACK],
[
    AC_REQUIRE([SFAC_LIB_PORT])
    AC_SUBST([SIPXTACK_LIBS], [-lsipXtack])
]) # SFAC_LIB_STACK


## sipXmediaLib 
# SFAC_LIB_MEDIA attempts to find the sf media library and include
# files by looking in /usr/[lib|include], /usr/local/[lib|include], and
# relative paths.
#
# If not found, the configure is aborted.  Otherwise, variables are defined
# for both the INC and LIB paths AND the paths are added to the CFLAGS, 
# CXXFLAGS, LDFLAGS, and LIBS.
AC_DEFUN([SFAC_LIB_MEDIA],
[
    AC_REQUIRE([SFAC_LIB_STACK])
    AC_SUBST([SIPXMEDIA_LIBS], [-lsipXmedia])
]) # SFAC_LIB_MEDIA



## sipXmediaAdapterLib 
# SFAC_LIB_MEDIAADAPTER attempts to find the sf media iadapter library and include
# files by looking in /usr/[lib|include], /usr/local/[lib|include], and
# relative paths.
#
# If not found, the configure is aborted.  Otherwise, variables are defined
# for both the INC and LIB paths AND the paths are added to the CFLAGS, 
# CXXFLAGS, LDFLAGS, and LIBS.
AC_DEFUN([SFAC_LIB_MEDIAADAPTER],
[
    AC_REQUIRE([SFAC_LIB_MEDIA])
    AC_SUBST([SIPXMEDIAADAPTER_LIBS], [-lsipXmediaProcessing])
]) # SFAC_LIB_MEDIAADAPTER


## Optionally compile in the GIPS library in the media subsystem
# (sipXmediaLib project) and executables that link it in
# Conditionally use the GIPS audio libraries
AC_DEFUN([CHECK_GIPSNEQ],
[
   AC_ARG_WITH(gipsneq,
      [  --with-gipsneq       Compile the media subsystem with the GIPS audio libraries
],
      compile_with_gips=yes)

   AC_MSG_CHECKING(if link in with gips NetEQ)

   if test x$compile_with_gips = xyes
   then
      AC_MSG_RESULT(yes)
      
      SFAC_SRCDIR_EXPAND

      AC_MSG_CHECKING(for gips includes)
      # Define HAVE_GIPS for c pre-processor
      GIPS_CPPFLAGS=-DHAVE_GIPS
      if test -e $withval/include/GIPS/Vendor_gips_typedefs.h
      then
         gips_dir=$withval
      elif test -e $abs_srcdir/../sipXbuild/vendors/gips/include/GIPS/Vendor_gips_typedefs.h
      then
         gips_dir=$abs_srcdir/../sipXbuild/vendors/gips
      else
         AC_MSG_ERROR(GIPS/Vendor_gips_typedefs.h not found)
      fi

      AC_MSG_RESULT($gips_dir)

      # Add GIPS include path
      GIPSINC=$gips_dir/include
      # Add GIPS objects to be linked in
      GIPS_NEQ_OBJS=$gips_dir/GIPS_SuSE_i386.a
      CPPFLAGS="$CPPFLAGS $GIPS_CPPFLAGS -I$GIPSINC"
      # GIPS needs to be at the end of the list
      #LIBS="$LIBS $GIPS_OBJS"

   else
      AC_MSG_RESULT(no)
   fi

   AC_SUBST(GIPSINC)
   AC_SUBST(GIPS_NEQ_OBJS)
   AC_SUBST(GIPS_CPPFLAGS)

   AC_SUBST(SIPXMEDIA_MP_LIBS, ["$SIPXMEDIALIB/libsipXmediaProcessing.la"])
]) # CHECK_GIPSNEQ


AC_DEFUN([CHECK_GIPSVE],
[
   AC_ARG_WITH(gipsve,
      [  --with-gipsve       Link to GIPS voice engine if --with-gipsve is set],
      link_with_gipsve=yes)

   AC_MSG_CHECKING(if linking to gips voice engine)

   if test x$link_with_gipsve = xyes
   then
      AC_MSG_RESULT(yes)
      
      SFAC_SRCDIR_EXPAND

      AC_MSG_CHECKING(for gips includes)

      if test -e $abs_srcdir/../sipXbuild/vendors/gips/VoiceEngine/interface/GipsVoiceEngineLib.h
      then
         gips_dir=$abs_srcdir/../sipXbuild/vendors/gips
      else
         AC_MSG_ERROR(sipXbuild/vendors/gips/VoiceEngine/interface/GipsVoiceEngineLib.h not found)
      fi

      AC_MSG_RESULT($gips_dir)

      # Add GIPS include path
      GIPSINC=$gips_dir/VoiceEngine/interface
      CPPFLAGS="$CPPFLAGS -I$gips_dir/include -I$GIPSINC"
      # Add GIPS objects to be linked in
      GIPS_VE_OBJS="$gips_dir/VoiceEngine/libraries/VoiceEngine_Linux_gcc.a"

   else
      AC_MSG_RESULT(no)
   fi

   AC_SUBST(GIPSINC)
   AC_SUBST(GIPS_VE_OBJS)

   AC_SUBST(SIPXMEDIA_VE_LIBS, ["$SIPXMEDIALIB/libsipXvoiceEngine.la"])

   AM_CONDITIONAL(BUILDVE, test x$link_with_gipsve = xyes)

]) # CHECK_GIPSVE

AC_DEFUN([CHECK_GIPSCE],
[
   AC_ARG_WITH(gipsce,
      [  --with-gipsce       Link to GIPS conference engine if --with-gipsce is set],
      link_with_gipsce=yes)

   AC_MSG_CHECKING(if linking to gips conference engine)

   if test x$link_with_gipsce = xyes
   then
      AC_MSG_RESULT(yes)
      
      SFAC_SRCDIR_EXPAND

      AC_MSG_CHECKING(for gips includes)

      if test -e $abs_srcdir/../sipXbuild/vendors/gips/ConferenceEngine/interface/ConferenceEngine.h
      then
         gips_dir=$abs_srcdir/../sipXbuild/vendors/gips
      else
         AC_MSG_ERROR(sipXbuild/vendors/gips/ConferenceEngine/interface/ConferenceEngine.h not found)
      fi

      AC_MSG_RESULT($gips_dir)

      # Add GIPS include path
      GIPSINC=$gips_dir/ConferenceEngine/interface
      CPPFLAGS="$CPPFLAGS -I$gips_dir/include -I$GIPSINC"
      # Add GIPS objects to be linked in
      GIPS_CE_OBJS="$gips_dir/ConferenceEngine/libraries/ConferenceEngine_Linux_gcc.a"

   else
      AC_MSG_RESULT(no)
   fi

   AC_SUBST(GIPSINC)
   AC_SUBST(GIPS_CE_OBJS)

   AC_SUBST(SIPXMEDIA_CE_LIBS, ["$SIPXMEDIALIB/libsipXconferenceEngine.la"])
   AM_CONDITIONAL(BUILDCE, test x$link_with_gipsce = xyes)

]) # CHECK_GIPSCE


## sipXcallLib
# SFAC_LIB_CALL attempts to find the sf call processing library and include
# files by looking in /usr/[lib|include], /usr/local/[lib|include], and
# relative paths.
#
# If not found, the configure is aborted.  Otherwise, variables are defined
# for both the INC and LIB paths AND the paths are added to the CFLAGS,
# CXXFLAGS, LDFLAGS, and LIBS.
AC_DEFUN([SFAC_LIB_CALL],
[
    AC_REQUIRE([SFAC_LIB_MEDIA])
    AC_SUBST([SIPXCALL_LIBS], [-lsipXcall])
]) # SFAC_LIB_CALL


## sipXcommserverLib

# SFAC_LIB_CALL attempts to find the sf communication server library and 
# include files by looking in /usr/[lib|include], /usr/local/[lib|include], 
# and relative paths.
#
# If not found, the configure is aborted.  Otherwise, variables are defined
# for both the INC and LIB paths AND the paths are added to the CFLAGS,
# CXXFLAGS, LDFLAGS, and LIBS.
AC_DEFUN([SFAC_LIB_COMMSERVER],
[
    AC_REQUIRE([SFAC_LIB_STACK])
    AC_SUBST([SIPXCOMMSERVER_LIBS], [-lsipXcommserver])
]) # SFAC_LIB_COMMSERVER


## resiprocate
# CHECK_RESIPROCATE attempts to find the resiprocate project tree
# 
# If not found, the configure is aborted.  Otherwise, variables are defined for:
# RESIPROCATE_PATH     - the top of the resiprocate tree
# RESIPROCATE_CFLAGS   
# RESIPROCATE_CXXFLAGS
# RESIPROCATE_LIBS
# RESIPROCATE_LDFLAGS
AC_DEFUN([CHECK_RESIPROCATE],
[
    AC_REQUIRE([SFAC_INIT_FLAGS])
    
    AC_ARG_WITH([resiprocate],
        [--with-resiprocate specifies the path to the top of a resiprocate project tree],
        [resiprocate_path=$withval],
        [resiprocate_path="$prefix /usr /usr/local"]
    )

    AC_ARG_WITH([resipobj],
        [--with-resipobj specifies the object directory name to use from resiprocate],
        [useresipobj=true; resipobj=$resiprocate_path/$withval],
        [useresipobj=false]
    )

    AC_MSG_CHECKING([for resiprocate includes])
    foundpath=NO
    for dir in $resiprocate_path ; do
        if test -f "$dir/include/resip/stack/SipStack.hxx"
        then
            foundpath=$dir/include;
            break;
        elif test -f "$dir/resip/stack/SipStack.hxx"
        then
            foundpath=$dir;
            break;
        fi;
    done
    if test x_$foundpath = x_NO
    then
       AC_MSG_ERROR([not found; searched '$resiprocate_path' for 'include/resip/stack/SipStack.hxx' or 'resip/stack/SipStack.hxx'])
    else
       AC_MSG_RESULT($foundpath)

       RESIPROCATE_PATH=$foundpath

       RESIPROCATE_CFLAGS="-I$RESIPROCATE_PATH"
       RESIPROCATE_CXXFLAGS="-I$RESIPROCATE_PATH"

       if test x$useresipobj = xtrue
       then
           RESIPROCATE_LDFLAGS=" -L$RESIPROCATE_PATH/resip/dum/$resipobj"
           RESIPROCATE_LDFLAGS=" $RESIPROCATE_LDFLAGS -L$RESIPROCATE_PATH/resip/stack/$resipobj"
           RESIPROCATE_LDFLAGS=" $RESIPROCATE_LDFLAGS -L$RESIPROCATE_PATH/rutil/$resipobj"
           RESIPROCATE_LDFLAGS=" $RESIPROCATE_LDFLAGS -L$RESIPROCATE_PATH/contrib/ares"
       else
           AC_MSG_CHECKING([for resiprocate libraries])
           foundpath=NO
           for dir in $resiprocate_path ; do
               if test -f "$dir/lib/libsipXresiprocateLib.la";
               then
                   foundpath=$dir/lib;
                   break;
               elif test -f "$dir/libsipXresiprocateLib.la";
               then
                   foundpath=$dir;
                   break;
               fi;
           done
           if test x_$foundpath = x_NO
           then
              AC_MSG_ERROR([not found; searched '$resiprocate_path' for 'lib/libsipXresiprocateLib.la' or 'libsipXresiprocateLib.la'])
           else
              AC_MSG_RESULT($foundpath)
              RESIPROCATE_LIBDIR=$foundpath
              RESIPROCATE_LDFLAGS=" -L$foundpath"
           fi
       fi

       RESIPROCATE_LIBS="${RESIPROCATE_LIBDIR}/libsipXresiprocateLib.la -ldum -lresip -lrutil -lares"

       AC_SUBST(RESIPROCATE_PATH)
       AC_SUBST(RESIPROCATE_CFLAGS)
       AC_SUBST(RESIPROCATE_CXXFLAGS)
       AC_SUBST(RESIPROCATE_LIBS)
       AC_SUBST(RESIPROCATE_LDFLAGS)
    fi
]) # CHECK_RESIPROCATE



##  Generic find of an include
#   Fed from AC_DEFUN([SFAC_INCLUDE_{module name here}],
#
# $1 - sample include file
# $2 - variable name (for overridding with --with-$2
# $3 - help text
# $4 - directory name (assumed parallel with this script)
AC_DEFUN([SFAC_ARG_WITH_INCLUDE],
[
    SFAC_SRCDIR_EXPAND()

    AC_MSG_CHECKING(for [$4] [($1)] includes)
    AC_ARG_WITH( [$2],
        [ [$3] ],
        [ include_path=$withval ],
        [ include_path="$includedir $prefix/include /usr/include /usr/local/include [$abs_srcdir]/../[$4]/include [$abs_srcdir]/../[$4]/interface [$abs_srcdir]/../[$4]/src/test"]
    )

    for dir in $include_path ; do
        if test -f "$dir/[$1]";
        then
            foundpath=$dir;
            break;
        fi;
    done
    if test x_$foundpath = x_; then
       AC_MSG_ERROR("'$1' not found; searched $include_path")
    fi
        

]) # SFAC_ARG_WITH_INCLUDE


##  Generic find of a library
#   Fed from AC_DEFUN([SFAC_LIB_{module name here}],
#
# $1 - sample lib file
# $2 - variable name (for overridding with --with-$2
# $3 - help text
# $4 - directory name (assumed parallel with this script)
AC_DEFUN([SFAC_ARG_WITH_LIB],
[
    SFAC_SRCDIR_EXPAND()

    AC_MSG_CHECKING(for [$4] [($1)] libraries)
    AC_ARG_WITH( [$2],
        [ [$3] ],
        [ lib_path=$withval ],
        [ lib_path="$libdir $prefix/lib /usr/lib /usr/local/lib `pwd`/../[$4]/src `pwd`/../[$4]/sipXmediaMediaProcessing/src `pwd`/../[$4]/src/test/sipxunit `pwd`/../[$4]/src/test/testlib" ]
    )
    foundpath=""
    for dir in $lib_path ; do
        if test -f "$dir/[$1]";
        then
            foundpath=$dir;
            break;
        fi;
    done
    if test x_$foundpath = x_; then
       AC_MSG_ERROR("'$1' not found; searched $lib_path")
    fi
]) # SFAC_ARG_WITH_LIB


AC_DEFUN([SFAC_SRCDIR_EXPAND], 
[
    abs_srcdir=`cd $srcdir && pwd`
])


AC_DEFUN([SFAC_FEATURE_SIP_TLS],
[
   AC_ARG_ENABLE(sip-tls, 
                 [  --enable-sip-tls        enable support for sips: and transport=tls (no)],
                 [], [enable_sip_tls=no])
   AC_MSG_CHECKING([Support for SIP over TLS])
   AC_MSG_RESULT(${enable_sip_tls})

   if test "${enable_sip_tls}" = "yes"
   then
      CFLAGS="-DSIP_TLS $CFLAGS"
      CXXFLAGS="-DSIP_TLS $CXXFLAGS"
   fi
])


AC_DEFUN([SFAC_FEATURE_SIPX_EZPHONE],
[
   AC_REQUIRE([CHECK_WXWIDGETS])

   AC_ARG_ENABLE(sipx-ezphone, 
                 [  --enable-sipx-ezphone    build the sipXezPhone example application (no)],
                 [], [enable_sipx_ezphone=no])
   AC_MSG_CHECKING([Building sipXezPhone])

   # If sipx-ezphone is requested, check for its prerequisites.
   if test x$enable_sipx_ezphone = xyes
   then
       if test x$enable_wxwidgets != xyes
       then
	      AC_MSG_ERROR([wxWidgets is required for sipXezPhone])
	      enable_sipx_ezphone=no
       fi
   fi

   AM_CONDITIONAL(BUILDEZPHONE, test x$enable_sipx_ezphone = xyes)

   AC_MSG_RESULT(${enable_sipx_ezphone})
])

AC_DEFUN([SFAC_FEATURE_DBTEST],
[
   AC_REQUIRE([CHECK_ODBC])

   AC_ARG_WITH(dbtests, 
               [  --with-dbtests=dbname run database unit tests (no)],
               [enable_dbtests=yes], 
               [enable_dbtests=no])
   AC_MSG_CHECKING([for enabling database unit tests])
   if test x$enable_dbtests = xyes
   then
     if test x$withval = x
     then
       SIPXTEST_DATABASE=SIPXDB-TEST
     else
       # Allow for --with-dbtests without parameters
       if test x$withval = xyes
       then
         SIPXTEST_DATABASE=SIPXDB-TEST
       else
         SIPXTEST_DATABASE=$withval
       fi
     fi
     AC_MSG_RESULT([${enable_dbtests} - using database $SIPXTEST_DATABASE])

     AC_MSG_CHECKING([for running PostgreSQL])

     if psql -l -U postgres &>/dev/null
     then
       AC_MSG_RESULT(running)
       # Run tests in a separate test database
       AC_SUBST(SIPXTEST_DATABASE)
     else
       AC_MSG_RESULT(not running - disabling test)
     enable_dbtests=no
     fi

   else
     AC_MSG_RESULT(${enable_dbtests})
   fi
])

# Place to store RPM output files
AC_DEFUN([SFAC_RPM_REPO],
[
  AC_ARG_ENABLE([rpm-repo],
    AC_HELP_STRING([--enable-rpm-repo=directory], 
      [Directory to create RPM repository for RPM targets, no default!]),
    [RPM_REPO=${enableval}],[RPM_REPO=""])

  if test "x$RPM_REPO" != "x"
  then
    mkdir -p "$RPM_REPO"
    RPM_REPO=`cd "$RPM_REPO"; pwd`
  fi
  AC_SUBST(RPM_REPO)
])
