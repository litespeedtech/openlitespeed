import json, logging, os, pwd, subprocess, sys
from stat import *

VERSION='0.0.1'

OPTION_CPU=0
OPTION_IO=1
OPTION_IOPS=2
OPTION_MEM=3
OPTION_TASKS=4

this = sys.modules[__name__]
this.logged = None

def init_logging():
    logging.basicConfig(format="%(asctime)s.%(msecs)03d" + " [%(levelname)s] %(message)s",
                        datefmt="%Y-%m-%d %H:%M:%S")
    
def server_root():
    return '/usr/local/lsws'

def get_conf_file(fileonly):
    return server_root() + "/lsns/conf/" + fileonly

def get_bin_file(fileonly):
    return server_root() + "/lsns/bin/" + fileonly

def fatal_error(msg):
    logging.error(msg)
    sys.exit(1)
    
def get_options():
    return ['cpu', 'io', 'iops', 'mem', 'tasks']

def get_user(param):
    if param.isdigit():
        try:
            user_info = pwd.getpwuid(int(param))
        except Exception as err:
            fatal_error('Error getting UID for %s: %s' % (param, err))
    else:
        try:
            user_info = pwd.getpwnam(param)
        except Exception as err:
            fatal_error('Error getting UID for %s: %s' % (param, err))
    if user_info.pw_uid < get_min_uid():
        fatal_error('Specified uid: %d < minimum uid: %d' % (user_info.pw_uid, get_min_uid()))
    return user_info

def get_users(uids):
    users = []
    for uid in uids:
        users.append(get_user(uid))
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
    fullfile = get_conf_file('lsns.conf')
    try:
        f = open(fullfile, 'r')
    except Exception as err:
        if this.logged is None:
            logging.info('Error opening %s: %s, continuing with default min uid %d' % (fullfile, err, get_def_min_uid()))
            this.logged = True
        return get_def_min_uid()
    try:
        uidstr = f.readline()
    except Exception as err:
        if this.logged is None:
            logging.info('Error reading %s: %s, continuing with default min uid %d' % (fullfile, err, get_def_min_uid()))
            this.logged = True
        uidstr = str(get_def_min_uid())
    f.close()
    logging.debug('Using min uid: %d' % int(uidstr))
    return int(uidstr)

def container_file():
    return server_root() + "/lsns/conf/lscntr.txt"

def ols_conf_file():
    return server_root() + "/conf/httpd_config.conf"

def lsws_conf_file():
    return server_root() + "/conf/httpd_config.xml"

def get_disabled_uid_file():
    return server_root() + '/lsns/conf/ns_disabled_uids.conf'

def get_disabled_uid_file():
    return server_root() + '/lsns/conf/ns_disabled_uids.conf'

def get_pkg_dir():
    return server_root() + '/lsns/conf/packages'

def pkg_to_filename(pkg):
    if not os.path.isdir(get_pkg_dir()):
        if not os.path.isdir(server_root()):
            fatal_error("Missing LiteSpeed high level installation directory")
        os.mkdir(get_pkg_dir(), mode=0o700)
    return get_pkg_dir() + '/%s.conf' % pkg

def ls_ok():
    if os.access(container_file(), 0) == 0:
        fatal_error("You must configure LiteSpeed for LiteSpeed Containers")

def touch_restart_external(file, desc):
    logging.debug("restart_external %s by touch: %s" % (desc, file))
    result = subprocess.run(['touch', file], capture_output=True, text=True)
    if result.returncode != 0:
        fatal_error('Error in running: touch, errors: ' + result.stdout + ' ' + result.stderr)

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
                    os.mkdir(dironly)
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

