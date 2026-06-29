import json, logging, os, pwd, re, subprocess, sys
from stat import *
from subprocess import PIPE

VERSION='0.0.5'
# 0.0.2: Updated for package commands
# 0.0.3: Updated for limited CloudLinux support
# 0.0.4: Updated for CageFS support
# 0.0.5: Updated for better CageFS identification

OPTION_CPU=0
OPTION_IO=1
OPTION_IOPS=2
OPTION_MEM=3
OPTION_TASKS=4

this = sys.modules[__name__]
this.logged = None
this.serverRoot = '/usr/local/lsws'
this.min_uid = 0
this.cl = False

def init_logging():
    logging.basicConfig(format="%(asctime)s.%(msecs)03d" + " [%(levelname)s] %(message)s",
                        datefmt="%Y-%m-%d %H:%M:%S")
    
def server_root():
    return this.serverRoot

def set_server_root(root):
    this.serverRoot = root

def get_conf_file(fileonly):
    return server_root() + "/lsns/conf/" + fileonly

def get_bin_file(fileonly):
    return server_root() + "/lsns/bin/" + fileonly

def fatal_error(msg):
    logging.error(msg)
    sys.exit(1)

# --- security validation helpers ---
_PKG_NAME_RE = re.compile(r'\A[A-Za-z0-9 _.\-]{1,64}\Z')
_NUMERIC_VAL_RE = re.compile(r'\A-?\d+[KMGT]?\Z', re.IGNORECASE)

def validate_pkg_name(pkg):
    """Allowlist package names to prevent path traversal in pkg_to_filename().
    cPanel/Plesk allow spaces in plan/package names (e.g. 'Default Domain').
    Reject anything with path separators, control chars, or other oddities."""
    if not isinstance(pkg, str) or not _PKG_NAME_RE.match(pkg):
        fatal_error('Invalid package name: %r' % pkg)
    # Belt-and-suspenders: explicitly reject path-traversal building blocks
    # even though the regex above already excludes `/` and null bytes.
    if '..' in pkg or pkg.strip() in ('', '.'):
        fatal_error('Invalid package name: %r' % pkg)
    return pkg

def validate_uid(uid):
    """Coerce uid to int; reject strings starting with '-' that would be parsed as flags."""
    try:
        n = int(uid)
    except (TypeError, ValueError):
        fatal_error('Invalid UID: %r' % uid)
    if n < 0:
        fatal_error('Invalid UID (negative): %r' % uid)
    return n

def validate_numeric_value(val):
    """Allow only numeric cgroup values (optionally suffixed K/M/G/T) before passing to subprocess."""
    s = str(val)
    if not _NUMERIC_VAL_RE.match(s):
        fatal_error('Invalid numeric value: %r' % val)
    return s

def validate_path_under(path, root):
    """Ensure realpath(path) is contained under realpath(root). Returns the realpath."""
    rp = os.path.realpath(path)
    rr = os.path.realpath(root)
    if rp != rr and not rp.startswith(rr + os.sep):
        fatal_error('Path escapes expected root: %r not under %r' % (path, root))
    return rp

def get_options():
    return ['cpu', 'io', 'iops', 'mem', 'tasks']

def get_user(param, no_fatal=False):
    if param.isdigit():
        try:
            user_info = pwd.getpwuid(int(param))
        except Exception as err:
            if not no_fatal:
                fatal_error('Error getting UID for %s: %s' % (param, err))
            logging.debug('Error getting UID for %s: %s' % (param, err))
            return None, False
    else:
        try:
            user_info = pwd.getpwnam(param)
        except Exception as err:
            if not no_fatal:
                fatal_error('Error getting name for %s: %s' % (param, err))
            logging.debug('Error getting name for %s: %s' % (param, err))
            return None, False
    if user_info.pw_uid < get_min_uid():
        if not no_fatal:
            fatal_error('Specified uid: %d < minimum uid: %d' % (user_info.pw_uid, get_min_uid()))
        logging.debug('Specified uid: %d < minimum uid: %d' % (user_info.pw_uid, get_min_uid()))
        return None, False
    return user_info, True

def get_users(uids, no_fatal=False):
    users = []
    for uid in uids:
        user_info, got = get_user(uid, no_fatal)
        if got:
            users.append(user_info)
    return users

def get_plesk():
    if os.path.exists("/etc/plesk-release"):
        return True
    return False

def get_def_min_uid():
    if get_plesk():
        return 10000
    return 1000

def get_min_uid():
    if this.min_uid != 0:
        return this.min_uid
    fullfile = get_conf_file('lsns.conf')
    try:
        f = open(fullfile, 'r')
    except Exception as err:
        if this.logged is None:
            logging.info('Error opening %s: %s, continuing with default min uid %d' % (fullfile, err, get_def_min_uid()))
            this.logged = True
        this.min_uid = get_def_min_uid()
        return this.min_uid
    try:
        uidstr = f.readline()
    except Exception as err:
        if this.logged is None:
            logging.info('Error reading %s: %s, continuing with default min uid %d' % (fullfile, err, get_def_min_uid()))
            this.logged = True
        uidstr = str(get_def_min_uid())
    f.close()
    try:
        this.min_uid = int(uidstr.strip())
    except (AttributeError, ValueError):
        fatal_error('Invalid min uid in %s: %r' % (fullfile, uidstr))
    logging.debug('Using min uid: %d' % this.min_uid)
    return this.min_uid

def getpwall():
    uid_dict = {}
    final_users = []
    users = pwd.getpwall()
    for user in users:
        if user.pw_uid < get_min_uid():
            continue
        if user.pw_uid in uid_dict:
            continue
        if user.pw_name == 'nobody':
            continue
        uid_dict[user.pw_uid] = user
        final_users.append(user)
    return final_users

def container_file():
    return server_root() + "/lsns/conf/lscntr.txt"

def ols_conf_file():
    return server_root() + "/conf/httpd_config.conf"

def lsws_conf_file():
    return server_root() + "/conf/httpd_config.xml"

def get_disabled_uid_file():
    return server_root() + '/lsns/conf/ns_disabled_uids.conf'

def get_pkg_dir():
    return server_root() + '/lsns/conf/packages'

def pkg_to_filename(pkg):
    # Path-traversal defence: reject any package name with path separators or
    # other unexpected characters before building a filesystem path from it.
    # Panel-supplied names (cPanel/Plesk) flow through here as root.
    # `*` is allowed only because lspkgctl uses it as a glob wildcard.
    if pkg != '*':
        validate_pkg_name(pkg)
    if not os.path.isdir(get_pkg_dir()):
        if not os.path.isdir(server_root()):
            fatal_error("Missing LiteSpeed high level installation directory")
        os.mkdir(get_pkg_dir(), mode=0o700)
    return get_pkg_dir() + '/%s.conf' % pkg

def get_package_size_file():
    return server_root() + '/lsns/conf/redis_package_size.conf'

def ls_ok():
    if not os.access(container_file(), os.F_OK):
        fatal_error("You must configure LiteSpeed for LiteSpeed Containers")

def ls_cl_ok():
    if os.access('/etc/cagefs/cagefs.mp', os.F_OK):
        this.cl = True
        return
    ls_ok()

def is_cl():
    return this.cl

def touch_restart_external(file, desc):
    """Create or update mtime of `file` WITHOUT following symlinks.
    Defends against unprivileged users planting a symlink at file (e.g. in $HOME)
    that would let root touch an arbitrary path."""
    logging.debug("restart_external %s by touch: %s" % (desc, file))
    try:
        # O_NOFOLLOW: refuse if final component is a symlink
        # O_CREAT: create if missing, with explicit 0o644 perms
        fd = os.open(file, os.O_WRONLY | os.O_CREAT | os.O_NOFOLLOW, 0o644)
    except OSError as err:
        fatal_error('Error touching %s: %s' % (file, err))
    try:
        os.utime(fd, None)
    finally:
        os.close(fd)

def restart_external(users, all):
    users_used = {}
    try:
        if os.path.getsize(container_file()) > 0:
            f = open(container_file(), 'r')
            data = json.load(f)
            f.close()
            for file in data['reset_list']:
                dironly = os.path.dirname(file)
                for user in users:
                    if user.pw_dir == dironly:
                        users_used[user.pw_name] = user
                        break
                if not os.path.exists(dironly):
                    os.mkdir(dironly, mode=0o755)
                touch_restart_external(file, "in lscntr.txt")
        if all:
            touch_restart_external('/usr/local/lsws/admin/tmp/.lsphp_restart.txt', "for all")
        else:
            for user in users:
                if user.pw_name in users_used:
                    continue
                touch_restart_external(user.pw_dir + '/.lsphp_restart.txt', "for home")
    except Exception as err:
        fatal_error('Error managing restart: %s' % err)

def get_devices():
    block_devices = {}
    statinfo_dev = {}
    files = os.listdir('/dev')
    devices = []
    for file in files:
        if (len(file) >= 4 and file[:4] == 'loop') or (len(file) >= 5 and file[:5] == 'cdrom') or (len(file) >= 2 and (file[:2] == 'dm' or file[:2] == 'sr')):
            continue
        filename = '/dev/' + file
        statinfo = os.stat(filename)
        if S_ISBLK(statinfo.st_mode) != 0:
            logging.debug('filename: ' + filename + ' statinfo: ' + str(statinfo))
            devices.append(filename)
            statinfo_dev[filename] = statinfo
    for device in devices:
        retry = True
        while retry:
            retry = False
            for inner in devices:
                if device == inner:
                    continue
                if len(device) < len(inner) and device == inner[:len(device)]:
                    devices.remove(inner)
                    retry = True
                    break
    for device in devices:
        major = os.major(statinfo_dev[device].st_rdev)
        minor = os.minor(statinfo_dev[device].st_rdev)
        if not major in block_devices:
            block_devices[major] = {}
        block_devices[major][minor] = device
    logging.debug('Final devices: ' + str(devices) + ' Block Devices: ' + str(block_devices))
    return devices, block_devices

def str_num_values(valstr):
    if valstr == '':
        return ''
    val = float(valstr)
    if val >= 1099511627776:
        divisor = 1099511627776
        qualifier = 'T'
    elif val >= 1073741824:
        divisor = 1073741824
        qualifier = 'G'
    elif val >= 1048576:
        divisor = 1048576
        qualifier = 'M'
    elif val >= 1024:
        divisor = 1024
        qualifier = 'K'
    else:
        return valstr
    if round(val / divisor) < 10:
        str_num = '%d.%d%s' % (val / divisor, int(round((val / (divisor / 10)) % 10)), qualifier)
    else:
        str_num = '%d%s' % (val / divisor, qualifier)
    if str(int_num_values(str_num)) == valstr:
        return str_num
    logging.debug("%s != %s use %s" % (str(int_num_values(str_num)), valstr, valstr))
    return valstr

def int_num_values(valstr):
    if valstr == '' or valstr == '-1':
        return -1
    if valstr.isdigit():
        return int(valstr)
    if len(valstr) < 2:
        fatal_error('Invalid value specification: %s, must be number optionally followed by T, G, M or K' % valstr)
    suffix = valstr[len(valstr) - 1].upper()
    if not suffix.isalpha():
        suffix = ''
        prefix = valstr
    else:
        prefix = valstr[:len(valstr) - 1]
    try:
        pref = float(prefix)
    except Exception:
        fatal_error('Invalid value specification (prefix): %s, must be number optionally followed by T, G, M or K' % valstr)
    if suffix == 'T':
        multiplier = 1099511627776
    elif suffix == 'G':
        multiplier = 1073741824
    elif suffix == 'M':
        multiplier = 1048576
    elif suffix == 'K':
        multiplier = 1024
    elif suffix == '':
        multiplier = 1
    else:
        fatal_error('Invalid value specification (suffix): %s, must be number optionally followed by T, G, M or K' % valstr)
    logging.debug("int_num %s, %f * %f = %d" % (valstr, pref, multiplier, (int)(pref * multiplier)))
    fl = pref * multiplier
    return int(fl)

