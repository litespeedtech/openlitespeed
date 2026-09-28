<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2018-2026 LiteSpeed Technologies, Inc.
 * *******************************************
 */

namespace Lsc\Wp;


use ZipArchive;

class Util
{

    /**
     * Prefix applied to every $_SESSION key this library owns.
     *
     * The library shares a PHP session with whichever panel plugin loaded it
     * (WHM, cPanel, Plesk, DirectAdmin), so bare keys such as "scanInfo" could
     * collide with a key belonging to the panel itself or to another extension
     * in the same session.  Namespacing them removes that class of collision.
     *
     * Always build keys through self::sessionKey() rather than writing the
     * prefixed name as a literal, so the prefix has exactly one definition.
     *
     * @since 1.17.12.0
     * @var string
     */
    const SESSION_KEY_PREFIX = 'lsc_';

    /**
     * Keys that are ALSO published under their old un-prefixed name for a
     * transition period, for the benefit of plugin versions that read the
     * session directly and predate the namespacing.
     *
     * REMOVE AFTER MARCH 2027.  Deleting this list (and the bindLegacyAlias()
     * call sites) completes the namespacing; nothing else has to change.
     *
     * Only "scanInfo" is listed, and the narrowness is the point.  Publishing
     * a bare name re-opens exactly the collision this prefix exists to close,
     * so a key earns a place here only if a shipped plugin really reads it:
     *
     *   - Plesk 2.5.0 reads $_SESSION['scanInfo'] directly.  It is the only
     *     such reader in any of our plugins, and without the alias an
     *     in-flight scan on that version is silently abandoned (it falls back
     *     to the Manage view) whenever the library is updated mid-scan.
     *   - Plesk 2.6.0+ reads the namespaced key and falls back to the bare
     *     one, so it works with or without this alias.
     *   - The WHM/cPanel and DirectAdmin plugins never touch these keys at
     *     all; they only construct the library's own view models.
     *
     * The other five keys this library owns (refreshInfo, unflagInfo,
     * verInfo, massDashNotifyInfo, massDashDisableInfo) have no external
     * reader whatsoever, so aliasing them would be pure collision risk for no
     * compatibility gain.  Do not add them.
     *
     * @since 1.17.12.0
     * @var string[]
     */
    const LEGACY_ALIASED_KEYS = [ 'scanInfo' ];

    /**
     * Returns $name namespaced with self::SESSION_KEY_PREFIX, for use as a
     * $_SESSION key.
     *
     * @since 1.17.12.0
     *
     * @param string $name  Un-prefixed key name, e.g. "scanInfo".
     *
     * @return string
     */
    public static function sessionKey( $name )
    {
        return self::SESSION_KEY_PREFIX . $name;
    }

    /**
     * Binds the legacy un-prefixed name to the same value as the namespaced
     * key, when $name is one of self::LEGACY_ALIASED_KEYS.
     *
     * The two names are bound *by reference* rather than copied, so a reader
     * of the old name sees every later mutation -- including the array_splice
     * batch consumption the scan performs across requests.  PHP preserves the
     * binding through session serialization (it is written as an "R:" back
     * reference), so this survives the request boundary under all three
     * session.serialize_handler settings.
     *
     * Call this immediately after taking a reference to a namespaced entry
     * that is about to be written.  It is a no-op for every key not on the
     * list, so it is safe to call unconditionally.
     *
     * No "is the key set yet?" guard is needed, and one was deliberately
     * removed: a reference assignment auto-vivifies both the subscript and
     * $_SESSION itself, so an absent key binds as null and is filled in by
     * the caller's own write a line later.  An earlier version pre-seeded the
     * key with null via array_key_exists(), which produced an identical
     * result for every input and an identical serialization -- while turning
     * a bare call on an unset $_SESSION from "binds successfully" into a
     * TypeError, because array_key_exists() rejects null where the reference
     * assignment would simply have created the array.  Repeat calls are
     * likewise harmless: re-binding an existing alias is a no-op.
     *
     * REMOVE AFTER MARCH 2027, together with self::LEGACY_ALIASED_KEYS.
     *
     * @since 1.17.12.0
     *
     * @param string $name  Un-prefixed key name, e.g. "scanInfo".
     *
     * @return void
     */
    public static function bindLegacyAlias( $name )
    {
        if ( !in_array($name, self::LEGACY_ALIASED_KEYS, true) ) {
            return;
        }

        $_SESSION[$name] = &$_SESSION[self::sessionKey($name)];
    }

    /**
     * Removes a session entry this library owns.
     *
     * The bare name is cleared only for the keys in
     * self::LEGACY_ALIASED_KEYS, where it is an alias this library published
     * itself and which must go when the entry it aliases does.  Because
     * bindLegacyAlias() binds the two names *by reference*, clearing only the
     * namespaced key would leave the bare name behind holding the very value
     * that was just cleared -- an old reader would then see a finished
     * operation as though it were still in progress, indefinitely.
     *
     * For every other key the bare name is deliberately left alone.  This
     * library never published it and never reads it, so removing it would
     * gain nothing and would destroy an identically-named key belonging to
     * the panel or to another extension in the shared session -- exactly the
     * collision the prefix exists to prevent.  A bare entry left over from a
     * pre-namespacing library is therefore allowed to expire with the
     * session; it is unread either way.
     *
     * Reads deliberately do not consult the bare key: the alias keeps old
     * readers working, so a read-side fallback would add nothing except a way
     * for a stale value to be preferred over a fresh one.
     *
     * @since 1.17.12.0
     *
     * @param string $name  Un-prefixed key name, e.g. "scanInfo".
     *
     * @return void
     */
    public static function unsetSessionKey( $name )
    {
        unset($_SESSION[self::sessionKey($name)]);

        if ( in_array($name, self::LEGACY_ALIASED_KEYS, true) ) {
            unset($_SESSION[$name]);
        }
    }

    /**
     *
     * @param string $tag
     *
     * @return string
     */
    public static function get_request_var( $tag )
    {
        if ( isset($_REQUEST[$tag]) ) {
            return trim($_REQUEST[$tag]);
        }

        /**
         * Request var not found in $_REQUEST, try checking POST and
         * QUERY_STRING environment variables.
         */
        if ( $_SERVER['REQUEST_METHOD'] === 'POST' ) {
            $querystring = urldecode(getenv('POST'));
        }
        else {
            $querystring = urldecode(getenv('QUERY_STRING'));
        }

        if ( $querystring != ''
                && preg_match("/(?:^|\?|&)$tag=([^&]+)/", $querystring, $m) ) {

            return trim($m[1]);
        }

        return null;
    }

    /**
     *
     * @param string $tag
     *
     * @return array
     */
    public static function get_request_list( $tag )
    {
        $varValue = null;

        if ( isset($_REQUEST[$tag]) ) {
            $varValue = $_REQUEST[$tag];
        }
        else {
            /**
             * Request var not found in $_REQUEST, try checking POST and
             * QUERY_STRING environment variables.
             */
            if ( $_SERVER['REQUEST_METHOD'] === 'POST' ) {
                $querystring = urldecode(getenv('POST'));
            }
            else {
                $querystring = urldecode(getenv('QUERY_STRING'));
            }

            if ( $querystring != ''
                    && preg_match_all("/(?:^|\?|&)$tag\[]=([^&]+)/", $querystring, $m) ) {

                $varValue = $m[1];
            }
        }

        return (is_array($varValue)) ? $varValue : null;
    }

    /**
     *
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     */
    public static function restartLsws()
    {
        Logger::info('Performing a Graceful Restart to apply changes...');

        /**
         * @noinspection PhpMethodParametersCountMismatchInspection  Suppress
         *     for PHP 5.x.
         */
        if ( php_uname('s') == 'FreeBSD' ) {
            $lswsCtl = '/usr/local/etc/rc.d/lsws.sh';
        }
        else {
            $lswsCtl = '/sbin/service lsws';
        }

        exec("$lswsCtl restart");
    }

    /**
     * @since 2.1.15
     *
     * @param int $startTime
     * @param int $timeout
     *
     * @return bool
     */
    public static function timedOut( $startTime, $timeout )
    {
        return ((time() - $startTime) > $timeout);
    }

    /**
     * This function is used to get the file owner by name. Useful in cases
     * where UID is not accepted or setting a files group to match its owner
     * (It is not safe to assume UID == GID or GID exists for username 'x').
     *
     * @since 2.2.0
     *
     * @param string $filepath
     *
     * @return array  Keys are id, name, group_id
     */
    public static function populateOwnerInfo( $filepath )
    {
        clearstatcache();
        $ownerID = fileowner($filepath);
        $ownerInfo = posix_getpwuid($ownerID);

        return array(
            'user_id'   => $ownerID,
            'user_name' => $ownerInfo['name'],
            'group_id'  => filegroup($filepath)
        );
    }

    /**
     *
     * @param string $file
     * @param string $owner
     * @param string $group
     */
    public static function changeUserGroup( $file, $owner, $group )
    {
        chown($file, $owner);
        chgrp($file, $group);
    }

    /**
     * Set file permissions of $file2 to match those of $file1.
     *
     * @since 2.2.0
     *
     * @param string $file1
     * @param string $file2
     */
    public static function matchPermissions( $file1, $file2 )
    {
        /**
         * convert fileperms() returned dec to oct
         */
        chmod($file2, (fileperms($file1) & 0777));
    }

    /**
     *
     * @since 1.14.3
     *
     * @param string $url
     * @param bool   $headerOnly
     *
     * @return string
     */
    public static function getUrlContentsUsingFileGetContents(
        $url,
        $headerOnly = false )
    {
        if ( ini_get('allow_url_fopen') ) {
            /**
             * silence warning when OpenSSL missing while getting LSCWP ver
             * file.
             */
            $url_content = @file_get_contents($url);

            if ( $url_content !== false ) {

                if ( $headerOnly ) {
                    return implode("\n", $http_response_header);
                }

                return $url_content;
            }
        }

        return '';
    }

    /**
     *
     * @since 1.14.3
     *
     * @param string $url
     * @param bool   $headerOnly
     *
     * @return string
     */
    public static function getUrlContentsUsingPhpCurl(
        $url,
        $headerOnly = false )
    {
        if ( function_exists('curl_version') ) {
            $ch = curl_init();

            curl_setopt_array(
                $ch,
                array(
                    CURLOPT_URL            => $url,
                    CURLOPT_RETURNTRANSFER => true,
                    CURLOPT_HEADER         => $headerOnly,
                    CURLOPT_NOBODY         => $headerOnly,
                    CURLOPT_HTTP_VERSION   => CURL_HTTP_VERSION_1_1
                )
            );

            $url_content = curl_exec($ch);
            curl_close($ch);

            if ( $url_content !== false ) {
                return $url_content;
            }
        }

        return '';
    }

    /**
     *
     * @since 1.14.3
     *
     * @param string $url
     * @param string $headerOnly
     *
     * @return string
     */
    public static function getUrlContentsUsingExecCurl(
        $url,
        $headerOnly = false )
    {
        $cmd = 'curl -s';

        if ( $headerOnly ) {
            $cmd .= ' -I';
        }

        exec("$cmd " . escapeshellarg($url), $output, $ret);

        if ( $ret === 0 ) {
            return implode("\n", $output);
        }

        return '';
    }

    /**
     *
     * @param string $url
     * @param bool   $headerOnly
     *
     * @return string
     */
    public static function get_url_contents( $url, $headerOnly = false )
    {
        $content = self::getUrlContentsUsingFileGetContents($url, $headerOnly);

        if ( $content != '' ) {
            return $content;
        }

        $content = self::getUrlContentsUsingPhpCurl($url, $headerOnly);

        if ( $content != '' ) {
            return $content;
        }

        return self::getUrlContentsUsingExecCurl($url, $headerOnly);
    }

    /**
     *
     * @param string $dir
     *
     * @return false|string
     */
    public static function DirectoryMd5( $dir )
    {
        if ( !is_dir($dir) ) {
            return false;
        }

        $fileMd5s = array();
        $d = dir($dir);

        while ( ($entry = $d->read()) !== false ) {

            if ( $entry != '.' && $entry != '..' ) {
                $currEntry = "$dir/$entry";

                if ( is_dir($currEntry) ) {
                    $fileMd5s[] = self::DirectoryMd5($currEntry);
                }
                else {
                    $fileMd5s[] = md5_file($currEntry);
                }
            }
        }

        $d->close();
        return md5(implode('', $fileMd5s));
    }

    /**
     *
     * @param string $file
     * @param string $backup
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    private static function matchFileSettings( $file, $backup )
    {
        clearstatcache();
        $ownerID = fileowner($file);
        $groupID = filegroup($file);

        if ( $ownerID === false || $groupID === false ) {
            Logger::debug("Could not get owner/group of file $file");

            unlink($backup);

            Logger::debug("Removed file $backup");
            return false;
        }

        self::changeUserGroup($backup, $ownerID, $groupID);
        self::matchPermissions($file, $backup);

        return true;
    }

    /**
     *
     * @param string $filepath
     * @param string $bak
     *
     * @return string
     */
    private static function getBackupSuffix(
        $filepath,
        $bak = '_lscachebak_orig' )
    {
        $i = 1;

        if ( file_exists($filepath . $bak) ) {
            $bak = sprintf("_lscachebak_%02d", $i);

            while ( file_exists($filepath . $bak) ) {
                $i++;
                $bak = sprintf("_lscachebak_%02d", $i);
            }
        }

        return $bak;
    }

    /**
     *
     * @param string $filepath
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::verbose() call.
     * @throws LSCMException  Thrown indirectly by self::matchFileSettings()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::info() call.
     */
    public static function createBackup( $filepath )
    {
        $backup = $filepath . self::getBackupSuffix($filepath);

        if ( !copy($filepath, $backup) ) {
            Logger::debug(
                "Could not backup file $filepath to location $backup"
            );

            return false;
        }

        Logger::verbose("Created file $backup");

        if ( !self::matchFileSettings($filepath, $backup) ) {
            Logger::debug(
                "Could not backup file $filepath to location $backup"
            );

            return false;
        }

        Logger::debug('Matched owner/group setting for both files');
        Logger::info(
            "Successfully backed up file $filepath to location $backup"
        );

        return true;
    }

    /**
     * Resolve $entryName relative to $realDest and report whether the result
     * stays confined to $realDest.
     *
     * realpath() cannot be used here because the extracted entry does not
     * exist yet, so each segment is walked manually, collapsing '.' and '..'.
     *
     * @since 1.17.11
     *
     * @param string $realDest   Absolute destination path, trailing slash.
     * @param string $entryName  Archive entry name.
     *
     * @return bool
     */
    private static function isEntryContained( $realDest, $entryName )
    {
        $parts = [];

        foreach ( explode('/', $realDest . $entryName) as $part ) {

            if ( $part === '' || $part === '.' ) {
                continue;
            }

            if ( $part === '..' ) {
                array_pop($parts);
            }
            else {
                $parts[] = $part;
            }
        }

        $resolvedPath = '/' . implode('/', $parts);

        return
            strncmp($realDest, "$resolvedPath/", strlen($realDest)) === 0;
    }

    /**
     * Report whether unzip(1) would write $entryName to disk verbatim, i.e.
     * at "$realDest$entryName".
     *
     * unzip(1) rewrites member names in several ways that cannot be reduced
     * to a single lexical rule, and each one desynchronises the post-
     * extraction mode mask from the path actually written:
     *
     *   - '.' and '..' components are removed rather than resolved (see the
     *     '-:' entry in unzip(1)), but only when intermediate -- as a *final*
     *     component they become '_' and '__', so 'g/..' is written as
     *     'g/__'.
     *   - Control characters are stripped unless '-^' is passed, so
     *     "ctl\x01.txt" is written as 'ctl.txt' and "tab\ttab.txt" as
     *     'tabtab.txt'.
     *   - High-byte names are charset-translated and do not survive
     *     byte-for-byte. This is deliberately *not* gated on the "version
     *     made by" host byte, because the host byte alone does not decide it.
     *     On unzip 6.00 a high-byte name is rewritten unconditionally under
     *     host bytes 0, 6 and 11, but it is *also* rewritten under the Unix
     *     host (3) when general purpose bit 11 marks the name UTF-8, the
     *     central header carries any extra field at all, and the locale
     *     charset cannot represent the name -- unzip(1) then escapes it to a
     *     "#Uxxxx" form. zip(1) produces exactly that combination for a
     *     non-ASCII name, since it sets bit 11 and always emits 0x5455 and
     *     0x7875 fields: a 'caf\xc3\xa9.txt' member is written as
     *     'caf#U00e9.txt' under LC_ALL=C or POSIX, and verbatim under
     *     LC_ALL=C.utf8. The deciding input is the caller's locale, which no
     *     amount of inspecting the archive can reveal, so high bytes are
     *     refused outright rather than gated on the host byte.
     *
     *     The cost of refusing them is nil for the archives this path
     *     handles: every member of the LSCWP plugin package and of the
     *     per-locale translation archives is plain ASCII. Checked against
     *     the live downloads for the zh_CN, ru_RU, ja and fr_FR locales --
     *     each holds exactly the .po/.mo/.l10n.php trio named
     *     'litespeed-cache-<locale>.<ext>', with the locale itself ASCII by
     *     construction -- and all four extract successfully through this
     *     fallback. A translated *name* never appears; the translated text
     *     is inside the files.
     *   - A '\' is converted to '/' when the host byte is MSDOS (0) and the
     *     name contains no '/', which unzip(1) reads as a DOS-style archive
     *     ("appears to use backslashes as path separators"). 'plug\a.txt'
     *     is then written as 'plug/a.txt'.
     *   - A trailing VMS version suffix is stripped: "by default the ';##'
     *     version numbers are stripped" unless '-V' is passed (see the '-V'
     *     entry in unzip(1)), so 'evil.php;1' is written as 'evil.php'. This
     *     is independent of the "version made by" host byte -- swept across
     *     hosts 0, 2, 3 and 11 on unzip 6.00, all four strip it -- and
     *     applies only at the very end of the name, so a directory member
     *     'p;1/' and its child 'p;1/a.txt' are both written verbatim.
     *
     * Rather than model all of that, names that would be rewritten are
     * refused outright: every printable ASCII byte except '/', '\' and ';'
     * round-trips unchanged, which covers any legitimate plugin archive, and
     * the check fails closed on anything else.
     *
     * Note that this only inspects the central header name. A replacement
     * name supplied out of band through an extra field is not visible here
     * and is handled by extraFieldRewritesName().
     *
     * The name in the *local* header is not inspected anywhere, and does not
     * need to be: when the two disagree unzip(1) warns ("mismatching 'local'
     * filename"), writes the member under the central name, and exits 1. The
     * central name is the one maskExtractedModes() looks up, so the mask
     * still lands on the file that was written -- but only because that pass
     * runs before the exit status is examined. Verified against unzip 6.00
     * with a 0x7075 field in the local header alone, including one naming
     * '../escaped.txt': the member is written as the central name at 0644 and
     * nothing lands outside the destination.
     *
     * @since 1.17.11
     *
     * @param string $entryName  Archive entry name.
     *
     * @return bool
     */
    private static function isEntryNameVerbatim( $entryName )
    {
        if ( $entryName === '' ) {
            return false;
        }

        /**
         * A trailing '/' marks a directory member and is written as such, so
         * it is dropped before the components are checked -- otherwise the
         * empty final component below would refuse every directory entry.
         */
        foreach ( explode('/', rtrim($entryName, '/')) as $part ) {

            /**
             * '\x20-\x2e\x30-\x3a\x3c-\x5b\x5d-\x7e' is printable ASCII
             * minus '/' (\x2f), which cannot appear inside a component,
             * minus '\' (\x5c), which unzip(1) may turn into one, and minus
             * ';' (\x3b), which introduces the VMS version suffix unzip(1)
             * strips. Anything outside it is a control character or a high
             * byte, both of which unzip(1) rewrites.
             *
             * Anchored with '\z', not '$'. '$' also matches immediately
             * before a trailing newline, so "ok.txt\n" would satisfy the
             * allowlist while unzip(1) strips the newline and writes
             * 'ok.txt'. maskExtractedModes() would then stat the
             * un-stripped name, find nothing, and leave the member with the
             * group/other write bits the archive asked for -- the exact
             * desync this check exists to prevent.
             *
             * The '.' and '..' cases are refused in every position, which is
             * wider than strictly necessary: unzip(1) rewrites them only as
             * a *final* component ('g/..' -> 'g/__', 'g/.' -> 'g/_'), while
             * an intermediate one is removed and the surviving path is one
             * the mask pass would still find ('./p/a.php' and 'p/./a.php'
             * both land at 'p/a.php'). Both positions are refused anyway,
             * because distinguishing them buys nothing: no writer of a
             * legitimate plugin package emits either. zip(1) normalises a
             * './' prefix away, and the archives this path actually handles
             * carry none -- checked across the litespeed-cache, dash-notifier,
             * akismet, classic-editor, hello-dolly and wordfence packages
             * (1172 members) and the LSCWP translation archives. A
             * ZipArchive-built archive is unaffected either way, since it
             * never reaches this fallback.
             *
             * ';' is refused in every position for the same reason, and is
             * wider than strictly necessary in the same way: only a trailing
             * ';##' on the final component is stripped. Refusing it anywhere
             * avoids having to decide what counts as "trailing" ('a;9;' is
             * written as 'a;9', so the suffix is the *last* one), and none of
             * the packages surveyed above contains a ';' in any member name.
             */
            if ( $part === ''
                    || $part === '.'
                    || $part === '..'
                    || !preg_match(
                        '/^[\x20-\x2e\x30-\x3a\x3c-\x5b\x5d-\x7e]+\z/',
                        $part
                    ) ) {

                return false;
            }
        }

        return true;
    }

    /**
     * Clear the group/other write bits that unzip(1) copied out of the
     * archive for each extracted $entryNames member under $realDest.
     *
     * unzip(1) applies the mode stored in the archive and does not consult
     * the process umask, so a member stored as 0666 lands group- and
     * world-writable. PluginVersion::prepareUserInstall() then copies it
     * into a user's plugin directory with '/bin/cp --preserve=mode', so the
     * archive decides what a tenant ends up with.
     *
     * ZipArchive::extractTo() does not merely umask that mode -- it ignores
     * the stored mode entirely, creating every file 0666 and every directory
     * 0777 as modified by the umask. Verified under strace against libzip
     * 1.7.3: extracting members stored 0700, 0777, 0755, 0666 and 0600
     * issues no chmod(2)/fchmod(2)/fchmodat(2) call at all and yields a
     * uniform 0644/0755 under umask 022. PHP is not even shown the mode --
     * ZipArchive::statIndex() exposes no mode field.
     *
     * So the two paths are brought to parity on the group/other write bits
     * specifically, which is what protects the tenant, and not on modes in
     * general: a member stored 0755 keeps its execute bits here while
     * ZipArchive would have flattened it to 0644. That divergence is
     * deliberate -- honouring an archive's execute bit is the behaviour
     * every other unzip consumer expects, and the write bits are the only
     * ones that hand out access.
     *
     * Masking is deliberately not delegated to umask(): the effective umask
     * is ambient state (lscmctl sets none of its own), so keying off it
     * would make the extracted tree depend on how the process was launched.
     * A fixed '~0022' does not.
     *
     * Note that unzip(1) already clears the setuid/setgid bits unless '-K' is
     * in the effective option set, so only the write bits need handling.
     * unzipFileWithCli() never passes '-K' and clears the environment
     * variables that could supply it.
     *
     * The sticky bit is the one exception, and it is implementation-specific:
     * Info-ZIP 6.00 discards it along with setuid/setgid, while busybox's
     * unzip applet preserves it even with no options or environment set at
     * all (a member stored 01755 comes out 01755, and a 041777 directory
     * member 01777). It is not added to the mask, because unlike
     * setuid/setgid it grants no access: on a directory it only restricts
     * deletion to the owner, which is more restrictive than the same mode
     * without it, and on a regular file Linux ignores it. busybox does strip
     * setuid/setgid on both files and directories, so the bits that would
     * matter are gone under either implementation.
     *
     * Note that not masking it is not the same as preserving it. chmod()
     * below is passed the low 0777 only, so where it fires at all it clears
     * every high bit as a side effect -- a busybox-extracted 01766 member
     * becomes 0744, losing the sticky bit, while a 01755 member keeps it
     * because no write bit needed clearing and the chmod() is skipped.
     * Measured both ways. Whether the bit survives therefore depends on the
     * member's write bits, which is acceptable only because the bit is
     * inert here; nothing should be built on it either way.
     *
     * Scope: only the archive's own members are masked. Intermediate
     * directories that no member names are created by unzip(1) from the
     * umask rather than from the archive -- identical to what
     * ZipArchive::extractTo() does with them -- so they are already at
     * parity and are left alone. Synthesising those parent paths here would
     * mean chmod()ing paths the preflight never validated.
     *
     * @since 1.17.11
     *
     * @param string   $realDest    Resolved destination, trailing slash.
     * @param string[] $entryNames  Archive entry names.
     *
     * @return void
     */
    private static function maskExtractedModes( $realDest, array $entryNames )
    {
        foreach ( $entryNames as $entryName ) {

            /**
             * Entry names have already been confirmed to resolve inside
             * $realDest, to contain no symlink member, and to be written by
             * unzip(1) verbatim, so the entry name is the path on disk and
             * needs no un-mangling here.
             *
             * Still guard with file_exists(), since a member may have been
             * skipped by unzip(1), and never follow a link.
             */
            $path = $realDest . rtrim($entryName, '/');

            if ( is_link($path) || !file_exists($path) ) {
                continue;
            }

            if ( ($perms = @fileperms($path)) === false ) {
                continue;
            }

            $masked = $perms & 0777 & ~0022;

            if ( $masked !== ($perms & 0777) ) {
                @chmod($path, $masked);
            }
        }
    }

    /**
     * Report whether the central header extra field block $extra can make
     * unzip(1) write the member under a name other than the central header
     * name.
     *
     * A 0x7075 "Unicode Path" field supplies a replacement name that
     * unzip(1) prefers over the header name, so 'ok.txt' can be stored in
     * the header while the member is written as 'evil.txt'. That name is
     * invisible to isEntryNameVerbatim(), which only ever sees the header
     * name, and it desynchronises maskExtractedModes() exactly as an
     * unzip(1)-side rename does.
     *
     * Establishing the exact conditions under which unzip(1) honours the
     * field (version byte 1, a name CRC matching the header name, non-empty
     * replacement, present in the *central* header) would mean modelling
     * rules that differ between unzip builds, so the field is instead
     * treated as making the name non-verbatim whenever it appears. No
     * producer of a legitimate plugin package emits one: ZipArchive and
     * PHP/Python writers emit no extra fields at all, and zip(1) signals
     * UTF-8 through general purpose bit 11 while emitting only 0x5455 and
     * 0x7875.
     *
     * A block that does not divide into fields exactly is also reported as
     * unsafe. Both unzip(1) and this walk step field-by-field over the same
     * bytes, so a field declaring a length that overruns the block leaves
     * neither able to say what follows it.
     *
     * @since 1.17.11
     *
     * @param string $extra  Central header extra field block.
     *
     * @return bool
     */
    private static function extraFieldRewritesName( $extra )
    {
        $len = strlen($extra);
        $pos = 0;

        while ( $pos + 4 <= $len ) {
            $field = unpack('vid/vsize', substr($extra, $pos, 4));

            if ( $field['id'] === 0x7075 ) {
                return true;
            }

            $pos += 4 + $field['size'];
        }

        return ($pos !== $len);
    }

    /**
     * Read the central directory of $zipFile and return one entry per member
     * as [ 'name' => string, 'isSymlink' => bool, 'nameRewritten' => bool,
     * 'isEncrypted' => bool ]. Returns false when the archive cannot be
     * parsed.
     *
     * The central directory is parsed directly rather than shelled out to
     * zipinfo because this runs precisely when ext-zip is missing, and
     * zipinfo's rendered mode column is not a reliable symlink oracle: it
     * derives the 'l' prefix from the "version made by" host byte, while
     * unzip(1) decides whether to create a symlink from the Unix mode bits.
     * A member marked with a non-Unix host still extracts as a symlink but
     * lists as, for example, 'RWED,RWED,RWED' for the VMS host.
     *
     * @since 1.17.11
     *
     * @param string $zipFile
     *
     * @return array[]|false
     */
    private static function getZipEntryInfo( $zipFile )
    {
        $size = @filesize($zipFile);

        if ( $size === false || $size < 22 ) {
            return false;
        }

        if ( ($fp = @fopen($zipFile, 'rb')) === false ) {
            return false;
        }

        /**
         * The end-of-central-directory record is last, followed only by an
         * archive comment of at most 0xFFFF bytes.
         */
        $tailLen = min($size, 0xFFFF + 22);
        fseek($fp, $size - $tailLen);
        $tail    = fread($fp, $tailLen);
        $eocdPos = strrpos($tail, "PK\x05\x06");

        if ( $eocdPos === false || $eocdPos + 22 > $tailLen ) {
            fclose($fp);

            return false;
        }

        $eocd = unpack(
            'vdiskNo/vcdDiskNo/vdiskEntries/vtotalEntries/VcdSize/VcdOffset',
            substr($tail, $eocdPos + 4, 16)
        );

        /**
         * Zip64 stores the real counts in a separate record. Fail closed
         * rather than misread the 32-bit sentinels; no plugin package is
         * anywhere near these limits.
         *
         * A saturated totalEntries is only a Zip64 sentinel when a Zip64
         * record is actually present. 0xFFFF is also the honest, in-range
         * value for an archive holding exactly 65535 members, which is not
         * Zip64 at all -- refusing it on the count alone rejected a
         * perfectly valid archive (zip(1), ZipArchive and unzip(1) all read
         * one back without complaint). The presence of the Zip64
         * end-of-central-directory *locator* is what distinguishes them: it
         * is a fixed 20 bytes immediately preceding the EOCD record, so it
         * is read at an absolute offset rather than out of $tail, which a
         * truncated read could cut short.
         *
         * The two 0xFFFFFFFF sentinels are not gated on the locator, because
         * an honest field can never hold that value: it would describe a
         * central directory starting at, or spanning, the 4GiB mark, which
         * only a Zip64 archive can express.
         *
         * The cdSize arm in particular is load-bearing and not merely
         * defense in depth, so do not fold it into the $size bound below.
         * That bound compares cdOffset + cdSize against the file size, and a
         * sparse archive can be large enough to satisfy it while cdSize is
         * still the sentinel -- at which point the fread() further down is
         * asked for 4GiB and PHP fatals on its memory limit instead of
         * returning false. Measured: with this arm removed, a 4GiB sparse
         * fixture whose cdSize is 0xFFFFFFFF (and which satisfies both the
         * $size bound and the cdOffset + cdSize equality) turns a clean
         * refusal into "Allowed memory size of 134217728 bytes exhausted".
         * Pinned by the "CLI fallback refuses a 0xFFFFFFFF cdSize without
         * allocating it" test.
         */
        $z64LocPos = $size - $tailLen + $eocdPos - 20;
        $haveZip64Locator = false;

        if ( $z64LocPos >= 0 ) {
            fseek($fp, $z64LocPos);
            $haveZip64Locator = (fread($fp, 4) === "PK\x06\x07");
        }

        if ( ($eocd['totalEntries'] === 0xFFFF && $haveZip64Locator)
                || $eocd['cdSize'] === 0xFFFFFFFF
                || $eocd['cdOffset'] === 0xFFFFFFFF ) {

            fclose($fp);

            return false;
        }

        /**
         * A cdSize of 0 is refused rather than read: it is the honest value
         * for an archive with no members at all, and the fread() below would
         * raise a ValueError on PHP 8 ("Argument #2 ($length) must be greater
         * than 0"), turning an odd archive into a fatal error. Refusing it
         * loses nothing -- unzip(1) itself exits 1 on a member-less archive
         * ("zipfile is empty"), so the CLI path could only ever have returned
         * false for one, and every caller treats false as a failed download.
         *
         * Note this is one of the few places the two extraction paths differ
         * in outcome rather than only in mechanism: ZipArchive::extractTo()
         * reports success for a member-less archive. Neither result is useful
         * -- the callers that check go on to look for an expected file and
         * fail there anyway (PluginVersion::wgetPlugin() requires
         * "$dir/litespeed-cache/litespeed-cache.php") -- so the divergence is
         * left as-is rather than papered over with a member-count special
         * case.
         */
        if ( $eocd['cdSize'] < 1
                || $eocd['cdOffset'] + $eocd['cdSize'] > $size ) {

            fclose($fp);

            return false;
        }

        /**
         * Require the central directory to end exactly where the
         * end-of-central-directory record begins.
         *
         * unzip(1) does not seek to the stored cdOffset. It derives the
         * central directory start from (eocd position - cdSize), treating any
         * difference from cdOffset as a prepended self-extracting stub. So an
         * archive whose cdOffset points at one central directory while
         * another sits immediately before the EOCD is read differently by the
         * two: the checks in unzipFileWithCli() would inspect the entries at
         * cdOffset while unzip(1) extracted the ones at (eocd - cdSize),
         * letting a symlink member through the preflight entirely.
         *
         * Both zip(1) and ZipArchive satisfy this equality, so nothing
         * legitimate is refused. Prepended-stub archives are refused, which
         * is correct here: this only ever handles plugin packages fetched
         * from wordpress.org.
         */
        $eocdFileOffset = $size - $tailLen + $eocdPos;

        if ( $eocd['cdOffset'] + $eocd['cdSize'] !== $eocdFileOffset ) {
            fclose($fp);

            return false;
        }

        fseek($fp, $eocd['cdOffset']);
        $cd = fread($fp, $eocd['cdSize']);
        fclose($fp);

        $entries = [];
        $offset  = 0;

        for ( $i = 0; $i < $eocd['totalEntries']; $i++ ) {

            if ( substr($cd, $offset, 4) !== "PK\x01\x02" ) {
                return false;
            }

            $header = @unpack(
                'vverMade/vverNeed/vflags/vmethod/vmTime/vmDate/Vcrc/'
                    . 'VcompSize/VuncompSize/vnameLen/vextraLen/vcommentLen/'
                    . 'vdiskStart/vintAttr/VextAttr/VlocalOffset',
                substr($cd, $offset + 4, 42)
            );

            if ( $header === false ) {
                return false;
            }

            $name = substr($cd, $offset + 46, $header['nameLen']);

            if ( strlen($name) !== $header['nameLen'] ) {
                return false;
            }

            $extra = substr(
                $cd,
                $offset + 46 + $header['nameLen'],
                $header['extraLen']
            );

            if ( strlen($extra) !== $header['extraLen'] ) {
                return false;
            }

            /**
             * The Unix mode lives in the high 16 bits of the external
             * attributes. Division rather than a shift keeps this correct on
             * 32-bit builds, where 'V' can widen to a float. The host byte is
             * deliberately ignored: unzip(1) keys off these bits, so testing
             * them alone cannot be sidestepped by forging the host.
             *
             * Only S_IFLNK is singled out. The other non-regular types are
             * not filtered because unzip(1) cannot create them: it links
             * symlink(2) but no mknod(2)/mkfifo(3) at all, and a member
             * stored as a device, FIFO or socket extracts as an ordinary
             * file (verified against unzip 6.00 -- 0020666, 0010666, 0060666
             * and 0140666 members all landed as plain files, then masked to
             * 0644 below). Rejecting them instead would also refuse
             * legitimate archives: Python's ZipFile.writestr() stores a mode
             * with no type bits at all (extAttr 0x01800000, i.e. S_IFMT 0),
             * so a "regular files only" test would fail closed on packages
             * built that way.
             */
            $mode = ( (int) floor($header['extAttr'] / 65536) ) & 0xF000;

            $entries[] = [
                'name'          => $name,
                'isSymlink'     => ($mode === 0xA000),
                'nameRewritten' => self::extraFieldRewritesName($extra),
                'isEncrypted'   => (bool) ($header['flags'] & 0x0001),
            ];

            $offset += 46 + $header['nameLen'] + $header['extraLen']
                + $header['commentLen'];
        }

        /**
         * Require the declared entry count to account for the whole central
         * directory. unzip(1) walks the central directory until it runs out
         * of records rather than stopping at totalEntries, so a record left
         * beyond the declared count is extracted while never being inspected
         * above -- the same preflight bypass as a mismatched cdOffset.
         */
        if ( $offset !== $eocd['cdSize'] ) {
            return false;
        }

        return $entries;
    }

    /**
     * Extract $zipFile into $dest using the CLI 'unzip' utility.
     *
     * Used when the ZipArchive extension is unavailable, which is the case
     * for the LiteSpeed admin PHP binary that lscmctl runs by default -- it
     * is built with '--disable-all' and never includes ext-zip.
     *
     * The archive is inspected before anything is written. Every entry must
     * resolve inside $realDest and must be written by unzip(1) under its own
     * name; symlink and encrypted members are rejected outright.
     *
     * Note that unzip(1) already strips '../' components and leading '/'
     * from member names by itself, so the containment pass is defense in
     * depth. The others are not. unzip(1) will happily create a symlink
     * pointing outside $dest, and prepareUserInstall() then copies that link
     * into a user's plugin directory. A rewritten name, meanwhile, desyncs
     * maskExtractedModes() from the paths on disk, leaving the member with
     * the group/other write bits the archive asked for. An encrypted member
     * makes unzip(1) prompt for a password on the terminal, hanging the
     * exec() below indefinitely whenever one is attached.
     *
     * Note also that unzip(1) has no '--' end-of-options marker. It reads a
     * leading '--' as its own "minus operator", which *cancels* options
     * rather than terminating them (see ENVIRONMENT OPTIONS in unzip(1)), so
     * passing it neither protects a '-'-leading $zipFile nor is a no-op:
     * 'unzip -oq -- -j.zip' fails to find the archive at all. What keeps the
     * argv unambiguous is that every caller builds $zipFile under
     * Context::LOCAL_PLUGIN_DIR, so it is always absolute and can never
     * begin with '-'.
     *
     * The inspection reads the archive, then exec()s unzip(1) on the same
     * path, so the two do not see one atomic snapshot. Nothing closes that
     * window here, and nothing needs to: $zipFile and $dest are always
     * under Context::LOCAL_PLUGIN_DIR ('/usr/src/litespeed-wp-plugin'),
     * created mode 0755 beneath root-owned '/usr/src', and the file is
     * written by the wget(1) immediately preceding each call. An
     * unprivileged user cannot replace a path in that tree, so there is no
     * one to win the race.
     *
     * @since 1.17.11
     *
     * @param string $zipFile
     * @param string $dest
     * @param string $realDest  Resolved $dest, trailing slash.
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    private static function unzipFileWithCli( $zipFile, $dest, $realDest )
    {
        if ( !function_exists('exec') ) {
            Logger::debug(
                "Could not unzip $zipFile: ZipArchive extension unavailable "
                    . "and exec() is disabled."
            );

            return false;
        }

        if ( ($entries = self::getZipEntryInfo($zipFile)) === false ) {
            Logger::debug(
                "Could not unzip $zipFile: unable to read archive contents."
            );

            return false;
        }

        $entryNames = [];

        foreach ( $entries as $entry ) {

            if ( $entry['isSymlink'] ) {
                Logger::debug(
                    "Symlink entry detected in $zipFile - extraction aborted."
                );

                return false;
            }

            /**
             * An encrypted member makes unzip(1) prompt for a password, and
             * it reads the reply from the terminal rather than from stdin --
             * redirecting stdin from /dev/null does not suppress it. With no
             * terminal attached the prompt fails immediately (exit 82), but
             * lscmctl run from a root shell does have one, and there exec()
             * blocks forever with the prompt sitting on a terminal nobody is
             * watching.
             *
             * Rejecting here rather than passing "-P ''" keeps the fallback
             * working with the unzip(1) builds that lack the flag, and this
             * is the exact set of archives that would prompt: unzip(1) takes
             * the encryption bit from the *central* header, so clearing it
             * only in the local header still prompts and is caught, while
             * clearing it centrally does not prompt. Strong/AES encryption
             * sets the same bit alongside its own.
             *
             * No legitimate plugin package from wordpress.org is encrypted.
             */
            if ( $entry['isEncrypted'] ) {
                Logger::debug(
                    "Encrypted entry detected in $zipFile - extraction "
                        . 'aborted.'
                );

                return false;
            }

            if ( !self::isEntryContained($realDest, $entry['name']) ) {
                /**
                 * The extractor is named because the ZipArchive branch logs
                 * the same "Zip-slip attempt detected" text. Without it a
                 * field log cannot say which of the two paths ran, and that
                 * is the first thing worth knowing here: reaching this line
                 * at all means the host had no ext-zip.
                 */
                Logger::debug(
                    "Zip-slip attempt detected in $zipFile by cli unzip "
                        . 'preflight - extraction aborted.'
                );

                return false;
            }

            if ( $entry['nameRewritten']
                    || !self::isEntryNameVerbatim($entry['name']) ) {

                Logger::debug(
                    "Entry name rewritten by unzip detected in $zipFile - "
                        . 'extraction aborted.'
                );

                return false;
            }

            $entryNames[] = $entry['name'];
        }

        $escZipFile = escapeshellarg($zipFile);

        $output = [];
        $retVar = -1;

        /**
         * UNZIP and UNZIPOPT are cleared because unzip(1) treats their
         * contents as "effectively the first options on the command line"
         * (see ENVIRONMENT OPTIONS in unzip(1)), and several of those
         * options undo the guarantees the preflight above establishes:
         * '-j' junks the directory part of every member, '-LL' lowercases
         * names, and '-K' keeps the setuid/setgid/sticky bits. The first two
         * leave maskExtractedModes() stat()ing a path unzip(1) did not
         * write, so the member keeps the group/other write bits the archive
         * asked for -- verified by extracting a clean-named 0666 member with
         * UNZIP=-j set, which landed at 0666 under a junked path. Assigning
         * an empty value is enough to neutralise both; unzip(1) does not
         * need them unset.
         *
         * '-d' in these variables is not a destination override: it lands in
         * the member list instead and the run fails, so only the mode/name
         * options above matter here.
         */
        exec(
            "UNZIP= UNZIPOPT= unzip -oq $escZipFile -d "
                . escapeshellarg($dest) . ' 2>&1',
            $output,
            $retVar
        );

        /**
         * Masked before the exit status is examined, not after. unzip(1)
         * writes members as it walks the archive and can still fail partway
         * through -- a local/central filename mismatch, for example, warns
         * and exits 1 having already written the member. Those members stay
         * on disk (nothing here removes them), so masking only on success
         * would leave them with the archive-supplied mode, which is the very
         * parity gap this pass exists to close.
         */
        self::maskExtractedModes($realDest, $entryNames);

        if ( $retVar !== 0 ) {
            Logger::debug(
                "Could not unzip $zipFile from cli with exit status $retVar: "
                    . implode(' ', $output)
            );

            return false;
        }

        return true;
    }

    /**
     *
     * @since 1.17.11  Falls back to the CLI 'unzip' utility
     *     when the ZipArchive extension is unavailable, restoring LSCWP
     *     download/upgrade on hosts running the bundled LiteSpeed admin PHP
     *     binary.
     *
     * @param string $zipFile
     * @param string $dest
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     * @throws LSCMException  Thrown indirectly by self::unzipFileWithCli()
     *     call.
     */
    public static function unzipFile( $zipFile, $dest )
    {
        $realDest = realpath($dest);

        if ( $realDest === false ) {
            Logger::debug(
                "Could not unzip $zipFile: destination directory does not exist."
            );

            return false;
        }

        $realDest = rtrim($realDest, '/') . '/';

        if ( class_exists('\ZipArchive') ) {
            $zipArchive = new ZipArchive();

            if ( $zipArchive->open($zipFile) === true ) {
                $slipDetected = false;

                for ( $i = 0; $i < $zipArchive->numFiles; $i++ ) {
                    $entryName = $zipArchive->getNameIndex($i);

                    if ( $entryName === false ) {
                        continue;
                    }

                    if ( !self::isEntryContained($realDest, $entryName) ) {
                        $slipDetected = true;
                        break;
                    }
                }

                if ( $slipDetected ) {
                    $zipArchive->close();
                    Logger::debug(
                        "Zip-slip attempt detected in $zipFile by ZipArchive "
                            . 'preflight - extraction aborted.'
                    );

                    return false;
                }

                $extracted = $zipArchive->extractTo($dest);
                $zipArchive->close();

                if ( $extracted ) {
                    return true;
                }
            }

            Logger::debug("Could not unzip $zipFile using ZipArchive.");

            return false;
        }

        return self::unzipFileWithCli($zipFile, $dest, $realDest);
    }

    /**
     * Returns true if $path is a non-empty absolute filesystem path
     * containing only characters safe for use in shell commands and Apache
     * configuration (alphanumerics, underscore, hyphen, period, forward
     * slash) and no '..' traversal segments.
     *
     * Use this to gate values before interpolating them into shell commands
     * or writing them into configuration files.
     *
     * @since 1.17.10
     *
     * @param string $path
     *
     * @return bool
     */
    public static function isSafeAbsPath( $path )
    {
        if ( !is_string($path) || $path === '' || $path[0] !== '/' ) {
            return false;
        }

        /**
         * Anchored with '\z', not '$'. '$' also matches immediately before a
         * trailing newline, so "/path/ok\n" passed this allowlist despite
         * the docblock promising only [A-Za-z0-9_./-]. Callers interpolate
         * the result into Apache config (setServerCacheRoot() writes a
         * "CacheRoot $cacheroot" line), where a trailing newline ends the
         * directive early. '\z' matches only at the very end of the subject.
         */
        if ( !preg_match('#^/[A-Za-z0-9_./\-]*\z#', $path) ) {
            return false;
        }

        $normalizedPath = rtrim($path, '/');

        if ( $normalizedPath === '' ) {
            return false;
        }

        foreach ( array_slice(explode('/', $normalizedPath), 1) as $segment ) {

            if ( $segment === '' || $segment === '.' || $segment === '..' ) {
                return false;
            }
        }

        return true;
    }

    /**
     * Check if a given directory is empty.
     *
     * @param string $dir
     *
     * @return bool
     */
    public static function is_dir_empty( $dir )
    {
        if ( !($handle = @opendir($dir)) ) {
            return true;
        }

        while ( ($entry = readdir($handle)) !== false ) {

            if ( $entry != '.' && $entry != '..' ) {
                return false;
            }
        }

        return true;
    }

    /**
     *
     * @param string $vhCacheRoot
     */
    public static function ensureVHCacheRootInCage( $vhCacheRoot )
    {
        $cageFsFile = '/etc/cagefs/cagefs.mp';

        if ( file_exists($cageFsFile) ) {

            if ( $vhCacheRoot[0] == '/' ) {
                $cageVhCacheRoot =
                    '%' . str_replace('/$vh_user', '', $vhCacheRoot);

                $matchFound = preg_grep(
                    "!^\s*" . str_replace('!', '\!', $cageVhCacheRoot) . "!im",
                    file($cageFsFile)
                );

                if ( !$matchFound ) {
                    file_put_contents(
                        $cageFsFile,
                        "\n$cageVhCacheRoot",
                        FILE_APPEND
                    );

                    exec('/usr/sbin/cagefsctl --remount-all');
                }
            }
        }
    }

    /**
     * Recursively a directory's contents and optionally the directory itself.
     *
     * @param string $dir        Directory path
     * @param bool   $keepParent Only remove directory contents when true.
     *
     * @return bool
     */
    public static function rrmdir( $dir, $keepParent = false )
    {
        if ( !is_string($dir)
                || $dir === ''
                || $dir === '/'
                || $dir === '.'
                || $dir === '..' ) {
            return false;
        }

        /**
         * V14 (CWE-59): if the top-level path itself is a symlink, never descend
         * through it. is_dir() follows symlinks, so recursing here would
         * enumerate and delete the link target's contents. Remove only the
         * link node instead.
         */
        if ( is_link($dir) ) {
            return $keepParent ? false : unlink($dir);
        }

        if ( is_dir($dir) ) {

            if ( ($matches = glob("$dir/*")) === false ) {
                return false;
            }

            foreach ( $matches as $file ) {

                /**
                 * V14 (CWE-59): never descend into symlinked directories. A symlink
                 * whose target is a directory makes is_dir() true, which would
                 * cause recursion to delete files outside the intended tree.
                 * Remove the link node itself instead.
                 */
                if ( is_link($file) ) {
                    unlink($file);
                }
                elseif ( is_dir($file) ) {
                    self::rrmdir($file);
                }
                else {
                    unlink($file);
                }
            }

            if ( !$keepParent ) {
                rmdir($dir);
            }

            return true;
        }

        return false;
    }

    /**
     * Wrapper for idn_to_utf8() function call to avoid "undefined" exceptions
     * when PHP intl module is not installed and enabled.
     *
     * @since 1.13.13.1
     *
     * @param string     $domain
     * @param int        $flags
     * @param int|null   $variant
     * @param array|null $idna_info
     *
     * @return false|string
     */
    public static function tryIdnToUtf8(
        $domain,
        $flags = 0,
        $variant = null,
        &$idna_info = null
    )
    {
        if ( empty($domain) || !function_exists('idn_to_utf8') ) {
            return $domain;
        }

        if ( defined('INTL_IDNA_VARIANT_UTS46') ) {

            if ( $variant == null ) {
                $variant = INTL_IDNA_VARIANT_UTS46;
            }

            return idn_to_utf8($domain, $flags, $variant, $idna_info);
        }

        return idn_to_utf8($domain, $flags);
    }

    /**
     * Wrapper for idn_to_ascii() function call to avoid "undefined" exceptions
     * when PHP intl module is not installed and enabled.
     *
     * @since 1.13.13.1
     *
     * @param string     $domain
     * @param int|null   $flags
     * @param int|null   $variant
     * @param array|null $idna_info
     *
     * @return false|string
     */
    public static function tryIdnToAscii(
        $domain,
        $flags = null,
        $variant = null,
        &$idna_info = null
    )
    {
        if ( empty($domain) || !function_exists('idn_to_ascii') ) {
            return $domain;
        }

        if ( $flags == null ) {
            $flags = IDNA_DEFAULT;
        }

        if ( defined('INTL_IDNA_VARIANT_UTS46') ) {

            if ( $variant == null ) {
                $variant = INTL_IDNA_VARIANT_UTS46;
            }

            return idn_to_ascii($domain, $flags, $variant, $idna_info);
        }

        return idn_to_ascii($domain, $flags);
    }

    /**
     * Version comparison function capable of properly comparing versions with
     * trailing ".0" groups such as '6.1' which is equal to '6.1.0' which is
     * equal to '6.1.000.0' etc.
     *
     * @since 1.14.2
     *
     * @param string      $ver1
     * @param string      $ver2
     * @param string|null $operator
     *
     * @return bool|int
     */
    public static function betterVersionCompare(
        $ver1,
        $ver2,
        $operator = null )
    {
        $pattern = '/(\.0+)+($|-)/';

        return version_compare(
            preg_replace($pattern, '', $ver1),
            preg_replace($pattern, '', $ver2),
            $operator
        );
    }

    /**
     *
     * @since 1.15.0.1
     *
     * @param string                           $constantName
     * @param array|bool|float|int|null|string $value
     * @param bool                             $caseInsensitive  Optional
     *     parameter used for define calls in PHP versions below 7.3.
     *
     * @return bool
     *
     * @noinspection PhpDeprecationInspection  Ignore deprecation of define()
     *     parameter $case_insensitive for PHP versions below 7.3.
     * @noinspection RedundantSuppression
     */
    public static function define_wrapper(
        $constantName,
        $value,
        $caseInsensitive = false )
    {
        if ( PHP_VERSION_ID < 70300 ) {
            return define($constantName, $value, $caseInsensitive);
        }
        else {
            return define($constantName, $value);
        }
    }

    /**
     *
     * @since 1.17.1.1
     *
     * @param int $wpStatus
     *
     * @return string[]  [ stateMsg => string, link => string ]
     */
    public static function getFatalErrorStateMessageAndLink( $wpStatus )
    {
        $stateMsg = $anchor = '';

        if ( $wpStatus & WPInstall::ST_ERR_EXECMD ) {
            $stateMsg = 'WordPress fatal error encountered during action '
                . 'execution. This is most likely caused by custom code in '
                . 'this WordPress installation.';
            $anchor   = '#fatal-error-encountered-during-action-execution';
        }
        if ( $wpStatus & WPInstall::ST_ERR_EXECMD_DB ) {
            $stateMsg = 'Error establishing WordPress database connection.';
        }
        elseif ( $wpStatus & WPInstall::ST_ERR_TIMEOUT ) {
            $stateMsg = 'Timeout occurred during action execution.';
            $anchor   = '#timeout-occurred-during-action-execution';
        }
        elseif ( $wpStatus & WPInstall::ST_ERR_SITEURL ) {
            $stateMsg = 'Could not retrieve WordPress siteURL.';
            $anchor   = '#could-not-retrieve-wordpress-siteurl';
        }
        elseif ( $wpStatus & WPInstall::ST_ERR_DOCROOT ) {
            $stateMsg = 'Could not match WordPress siteURL to a known '
                . 'control panel docroot.';
            $anchor   = '#could-not-match-wordpress-siteurl-to-a-known-'
                . 'cpanel-docroot';
        }
        elseif ( $wpStatus & WPInstall::ST_ERR_WPCONFIG ) {
            $stateMsg = 'Could not find a valid wp-config.php file.';
            $anchor   = '#could-not-find-a-valid-wp-configphp-file';
        }

        $stateMsg .= ' Click for more information.';

        return array(
            'stateMsg' => $stateMsg,
            'link'     => 'https://docs.litespeedtech.com/lsws/cp/cpanel/'
                . "whm-litespeed-plugin/troubleshooting/$anchor"
        );
    }

    /**
     * Locate a usable CA-certificate bundle in a panel-agnostic way.
     *
     * Probes well-known locations on the major Linux distributions and
     * supported control panels in priority order (OS-level bundles are
     * preferred over panel-managed ones). Returns the first readable path,
     * or '' when nothing is found (callers should then rely on the tool's
     * built-in default trust store).
     *
     * @return string  Absolute path to a CA bundle, or '' if none found.
     */
    public static function getSystemCaBundle()
    {
        static $resolved = null;

        if ( $resolved !== null ) {
            return $resolved;
        }

        $candidates = array(
            // OS-level bundles — preferred; kept current by the distro.
            '/etc/ssl/certs/ca-certificates.crt',               // Debian/Ubuntu/Alpine
            '/etc/pki/tls/certs/ca-bundle.crt',                 // RHEL/CentOS/AlmaLinux/Rocky
            '/etc/ssl/cert.pem',                                 // FreeBSD/OpenBSD/macOS
            '/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem',// RHEL update-ca-trust
            '/etc/ssl/ca-bundle.pem',                            // SUSE/openSUSE
            // Panel-managed bundles — fallback when OS bundle is absent.
            '/usr/local/cpanel/3rdparty/share/ca-bundle/curl-ca-bundle.crt',
            '/opt/psa/var/certificates/ca-bundle.crt',           // Plesk
            '/usr/local/directadmin/conf/carootcert.pem',        // DirectAdmin
        );

        foreach ( $candidates as $path ) {
            if ( is_file($path) && is_readable($path) ) {
                return $resolved = $path;
            }
        }

        return $resolved = '';
    }

    /**
     * Build the wget '--ca-certificate=...' argument string.
     *
     * Returns the shell-quoted flag with a trailing space when a bundle is
     * found, or '' when nothing was found (callers should omit the flag and
     * rely on the system default trust store — never pass
     * --no-check-certificate).
     *
     * @return string
     */
    public static function getWgetCaArg()
    {
        $bundle = self::getSystemCaBundle();

        return $bundle === ''
            ? ''
            : '--ca-certificate=' . escapeshellarg($bundle) . ' ';
    }

    /**
     * Build the curl '--cacert ...' argument string.
     *
     * Returns the shell-quoted flag with a trailing space when a bundle is
     * found, or '' when nothing was found (callers should omit the flag and
     * rely on the system default trust store — never pass --insecure).
     *
     * @return string
     */
    public static function getCurlCaArg()
    {
        $bundle = self::getSystemCaBundle();

        return $bundle === ''
            ? ''
            : '--cacert ' . escapeshellarg($bundle) . ' ';
    }

}
