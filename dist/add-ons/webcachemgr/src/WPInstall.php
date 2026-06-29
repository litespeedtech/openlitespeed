<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2026
 * *******************************************
 */

namespace Lsc\Wp;

use Lsc\Wp\Panel\ControlPanel;
use Lsc\Wp\Panel\PhpBinaryParts;
use Lsc\Wp\Context\Context;

class WPInstall
{

    /**
     * @var int
     */
    const ST_PLUGIN_ACTIVE = 1;

    /**
     * @var int
     */
    const ST_PLUGIN_INACTIVE = 2;

    /**
     * @var int
     */
    const ST_LSC_ADVCACHE_DEFINED = 4;

    /**
     * @var int
     */
    const ST_FLAGGED = 8;

    /**
     * @var int
     */
    const ST_ERR_SITEURL = 16;

    /**
     * @var int
     */
    const ST_ERR_DOCROOT = 32;

    /**
     * @var int
     */
    const ST_ERR_EXECMD = 64;

    /**
     * @var int
     */
    const ST_ERR_TIMEOUT = 128;

    /**
     * @var int
     */
    const ST_ERR_EXECMD_DB = 256;

    /**
     * @var int
     */
    const ST_ERR_WPCONFIG = 1024;

    /**
     * @var int
     */
    const ST_ERR_REMOVE = 2048;

    /**
     * @var string
     */
    const FLD_STATUS = 'status';

    /**
     * @var string
     */
    const FLD_DOCROOT = 'docroot';

    /**
     * @var string
     */
    const FLD_SERVERNAME = 'server_name';

    /**
     * @var string
     */
    const FLD_SITEURL = 'site_url';

    /**
     * @since 1.17.10  V9.1 — expected-owner uid persisted so
     *     Layer 2 (owner equality check) survives data-file serialization and
     *     fires on data-file-loaded installs in the root-context flag path.
     *
     * @var string
     */
    const FLD_EXPECTED_OWNER_UID = 'expected_owner_uid';

    /**
     * @since 1.17.10  V9.1 — expected-owner gid persisted.
     *
     * @var string
     */
    const FLD_EXPECTED_OWNER_GID = 'expected_owner_gid';

    /**
     * @var string
     */
    const FLAG_FILE = '.litespeed_flag';

    /**
     * @var string
     */
    const FLAG_NEW_LSCWP = '.lscm_new_lscwp';

    /**
     * @var string
     */
    protected $path;

    /**
     * @var array
     */
    protected $data;

    /**
     * @since 1.17.10
     * @var PhpBinaryParts|null
     */
    protected $phpBinaryParts = null;

    /**
     * @deprecated since 1.17.10  Use $phpBinaryParts instead.
     *
     * @var null|string
     */
    protected $phpBinary = null;

    /**
     * @var null|string
     */
    protected $suCmd = null;

    /**
     * @var null|array  Keys are 'user_id', 'user_name', 'group_id'
     */
    protected $ownerInfo = null;

    /**
     * @var bool
     */
    protected $changed = false;

    /**
     * @var bool
     */
    protected $refreshed = false;

    /**
     * @var null|string
     */
    protected $wpConfigFile = '';

    /**
     * @var int
     */
    protected $cmdStatus = 0;

    /**
     * @var string
     */
    protected $cmdMsg = '';

    /**
     * @since 1.17.10  V9 expected-owner binding (cross-tenant
     *     TOCTOU defence). Captured from realpath(docroot) ownership at scan
     *     time; null when the install was not created via a scan operation.
     *
     * @var int|null
     */
    protected $expectedOwnerUid = null;

    /**
     * @since 1.17.10  V9 expected-owner binding.
     *
     * @var int|null
     */
    protected $expectedOwnerGid = null;

    /**
     *
     * @param string $path
     */
    public function __construct( $path )
    {
        $this->init($path);
    }

    /**
     *
     * @param string $path
     */
    protected function init( $path )
    {
        if ( ($realPath = realpath($path)) === false ) {
            $this->path = $path;
        }
        else {
            $this->path = $realPath;
        }

        $this->data = array(
            self::FLD_STATUS             => 0,
            self::FLD_DOCROOT            => null,
            self::FLD_SERVERNAME         => null,
            self::FLD_SITEURL            => null,
            self::FLD_EXPECTED_OWNER_UID => null,
            self::FLD_EXPECTED_OWNER_GID => null,
        );
    }

    /**
     *
     * @return string
     */
    public function __toString()
    {
        if ( $this->data[self::FLD_SITEURL] ) {
            $siteUrl = Util::tryIdnToUtf8($this->data[self::FLD_SITEURL]);
        }
        else {
            $siteUrl = '';
        }

        return sprintf(
            "%s (status=%d docroot=%s siteurl=%s)",
            $this->path,
            $this->data[self::FLD_STATUS],
            $this->data[self::FLD_DOCROOT],
            $siteUrl
        );
    }

    /**
     *
     * @return string|null
     */
    public function getDocRoot()
    {
        return $this->getData(self::FLD_DOCROOT);
    }

    /**
     *
     * @param string $docRoot
     *
     * @return bool
     */
    public function setDocRoot( $docRoot )
    {
        return $this->setData(self::FLD_DOCROOT, $docRoot);
    }

    /**
     *
     * @return string|null
     */
    public function getServerName()
    {
        return $this->getData(self::FLD_SERVERNAME);
    }

    /**
     *
     * @param string $serverName
     *
     * @return bool
     */
    public function setServerName( $serverName )
    {
        return $this->setData(
            self::FLD_SERVERNAME,
            Util::tryIdnToAscii((string)$serverName)
        );
    }

    /**
     * Note: Temporary function name until existing deprecated setSiteUrl()
     * function is removed.
     *
     * @param string $siteUrl
     *
     * @return bool
     */
    public function setSiteUrlDirect( $siteUrl )
    {
        return $this->setData(
            self::FLD_SITEURL,
            Util::tryIdnToAscii((string)$siteUrl)
        );
    }

    /**
     *
     * @param int $status
     *
     * @return bool
     */
    public function setStatus( $status )
    {
        return $this->setData(self::FLD_STATUS, $status);
    }

    /**
     *
     * @return int
     */
    public function getStatus()
    {
        return $this->getData(self::FLD_STATUS);
    }

    /**
     *
     * @param string $field
     *
     * @return mixed|null
     */
    public function getData( $field = '' )
    {
        if ( !$field ) {
            return $this->data;
        }

        if ( isset($this->data[$field]) ) {
            return $this->data[$field];
        }

        /**
         * Error out
         */
        return null;
    }

    /**
     *
     * @param string $field
     * @param mixed  $value
     *
     * @return bool
     */
    protected function setData( $field, $value )
    {
        if ( $this->data[$field] !== $value ) {
            $this->changed = true;
            $this->data[$field] = $value;

            return true;
        }

        return false;
    }

    /**
     * Calling from unserialized data.
     *
     * @since 1.17.10  V9.1 — hydrate expectedOwnerUid/Gid from
     *     FLD_EXPECTED_OWNER_UID/GID when present so Layer 2 (expected-owner
     *     equality check) fires on data-file-loaded installs in the
     *     root-context flag path.
     *
     * @param array $data
     */
    public function initData( array $data )
    {
        /**
         * V9.2 — initialize expected-owner keys that may be absent from
         * pre-V9.1 data-file records. setData() assumes every key in $field
         * exists in $this->data; without this guard, calling setExpectedOwner()
         * on a legacy-loaded install would trigger an undefined-index notice.
         */
        if ( !array_key_exists(self::FLD_EXPECTED_OWNER_UID, $data) ) {
            $data[self::FLD_EXPECTED_OWNER_UID] = null;
        }

        if ( !array_key_exists(self::FLD_EXPECTED_OWNER_GID, $data) ) {
            $data[self::FLD_EXPECTED_OWNER_GID] = null;
        }

        $this->data = $data;

        if ( isset($data[self::FLD_EXPECTED_OWNER_UID]) ) {
            $this->expectedOwnerUid = (int) $data[self::FLD_EXPECTED_OWNER_UID];
        }

        if ( isset($data[self::FLD_EXPECTED_OWNER_GID]) ) {
            $this->expectedOwnerGid = (int) $data[self::FLD_EXPECTED_OWNER_GID];
        }
    }

    /**
     *
     * @return string
     */
    public function getPath()
    {
        return $this->path;
    }

    /**
     * Bind the cPanel-assigned tenant uid/gid of the docroot ancestor that
     * canonically contained $this->path at scan time.  Used by
     * addUserFlagFile() (root context) to fail-closed on cross-tenant
     * intermediate-directory symlink swaps: if the install-directory owner
     * at write time does not match the panel-assigned docroot owner captured
     * here, the privilege drop is refused.
     *
     * Called by WPInstallStorage::scan() after construction.  The binding is
     * persisted via FLD_EXPECTED_OWNER_UID / FLD_EXPECTED_OWNER_GID so it
     * survives data-file serialization; initData() hydrates the in-memory
     * properties when the install is reloaded, making Layer 2 effective on
     * the data-file-loaded root-context flag path.
     *
     * Paths that were never run through scan() (bare new WPInstall() in
     * doWPInstallAction(), and the scan2/addNewWPInstall() path that loses
     * docroot context) still carry null and remain Layer-1/Layer-3 reliant.
     *
     * @since 1.17.10
     * @since 1.17.10  V9.1 — now persisted via $data fields.
     *
     * @param int $uid  lstat() uid of realpath(docroot).
     * @param int $gid  lstat() gid of realpath(docroot).
     */
    public function setExpectedOwner( $uid, $gid )
    {
        $this->expectedOwnerUid = (int) $uid;
        $this->expectedOwnerGid = (int) $gid;
        $this->setData(self::FLD_EXPECTED_OWNER_UID, (int) $uid);
        $this->setData(self::FLD_EXPECTED_OWNER_GID, (int) $gid);
    }

    /**
     *
     * @return bool
     */
    public function shouldRemove()
    {
        return (bool)(($this->getStatus() & self::ST_ERR_REMOVE));
    }

    /**
     *
     * @return bool
     */
    public function hasFlagFile()
    {
        return file_exists("$this->path/" . self::FLAG_FILE);
    }

    /**
     *
     * @return bool
     */
    public function hasNewLscwpFlagFile()
    {
        return file_exists("$this->path/" . self::FLAG_NEW_LSCWP);
    }

    /**
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::notice() call.
     * @throws LSCMException  Thrown indirectly by $this->addUserFlagFile()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::uiError() call.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     */
    public function hasValidPath()
    {
        if ( !is_dir($this->path) || !is_dir("$this->path/wp-admin") ) {
            $this->setStatusBit(self::ST_ERR_REMOVE);

            $msg = "$this->path - Could not be found and has been removed from "
                . 'Cache Manager list.';
            Logger::uiError($msg);
            Logger::notice($msg);

            return false;
        }

        if ( $this->getWpConfigFile() == null ) {
            $this->setStatusBit(self::ST_ERR_WPCONFIG);
            $this->addUserFlagFile(false);

            $msg = "$this->path - Could not find a valid wp-config.php file. "
                . 'Install has been flagged.';
            Logger::uiError($msg);
            Logger::error($msg);

            return false;
        }

        return true;
    }

    /**
     * Set the provided status bit.
     *
     * @param int $bit
     */
    public function setStatusBit( $bit )
    {
        $this->setStatus(($this->getStatus() | $bit));
    }

    /**
     * Unset the provided status bit.
     *
     * @param int $bit
     */
    public function unsetStatusBit( $bit )
    {
        $this->setStatus(($this->getStatus() & ~$bit ));
    }

    /**
     *
     * @deprecated 1.9.5  Deprecated to avoid confusion with $this->cmdStatus
     *     and $this->cmdMsg related functions. Use $this->setStatus() instead.
     *
     * @param int $newStatus
     */
    public function updateCommandStatus( $newStatus )
    {
        $this->setData(self::FLD_STATUS, $newStatus);
    }

    /**
     *
     * @return null|string
     */
    public function getWpConfigFile()
    {
        if ( $this->wpConfigFile === '' ) {
            $file = "$this->path/wp-config.php";

            if ( !file_exists($file) ) {
                /**
                 *  check parent dir
                 */
                $parentDir = dirname($this->path);
                $file = "$parentDir/wp-config.php";

                if ( !file_exists($file)
                        || file_exists("$parentDir/wp-settings.php") ) {

                    /**
                     * If wp-config moved up, in same dir should NOT have
                     * wp-settings
                     */
                    $file = null;
                }
            }

            $this->wpConfigFile = $file;
        }

        return $this->wpConfigFile;
    }

    /**
     * Takes a WordPress site URL and uses it to populate serverName, siteUrl,
     * and docRoot data. If a matching docRoot cannot be found using the
     * serverName, the installation will be flagged and an ST_ERR_DOCROOT status
     * set.
     *
     * @param string $url
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance()->mapDocRoot() call.
     * @throws LSCMException  Thrown indirectly by $this->addUserFlagFile()
     *     call.
     * @throws LSCMException  Thrown indirectly by Logger::error() call.
     */
    public function populateDataFromUrl( $url )
    {
        /** @noinspection HttpUrlsUsage */
        $parseSafeUrl =
            (preg_match('#^https?://#', $url)) ? $url : "http://$url";

        $info = parse_url($parseSafeUrl);

        $serverName = isset($info['host'])
            ? Util::tryIdnToAscii($info['host'])
            : null;

        $this->setData(self::FLD_SERVERNAME, $serverName);

        $siteUrlTrim = $serverName;

        if ( isset($info['path']) ) {
            $siteUrlTrim .= $info['path'];
        }

        $this->setData(self::FLD_SITEURL, $siteUrlTrim);

        $docRoot = ControlPanel::getClassInstance()->mapDocRoot($serverName);
        $this->setData(self::FLD_DOCROOT, $docRoot);

        if ( $docRoot === null ) {
            $this->setStatus(self::ST_ERR_DOCROOT);
            $this->addUserFlagFile(false);

            $msg = "$this->path - Could not find matching document root for "
                . "WP siteurl/servername $serverName.";

            $this->setCmdStatusAndMsg(UserCommand::EXIT_ERROR, $msg);
            Logger::error($msg);
            return false;
        }

        return true;
    }

    /**
     * Deprecated 06/18/19. Renamed to populateDataFromUrl().
     *
     * @deprecated
     *
     * @param string $siteUrl
     *
     * @return bool
     *
     * @throws LSCMException  Thrown indirectly by $this->populateDataFromUrl()
     *     call.
     */
    public function setSiteUrl( $siteUrl )
    {
        return $this->populateDataFromUrl($siteUrl);
    }

    /**
     * Adds the flag file to an installation.
     *
     * The flag file lives inside an installation directory that is owned and
     * writable by an untrusted account, yet this method may run as root (when
     * $runningAsUser is false). It is therefore hardened against symlink/TOCTOU
     * attacks (CWE-59 / CWE-367):
     *
     *   When called as root ($runningAsUser === false):
     *     Both the unlink and the fopen('xb') are performed after dropping
     *     effective credentials to the install owner via dropPrivileges() (V8).
     *     Writing as the filesystem owner of the directory closes the
     *     intermediate-component TOCTOU: if a parent directory is swapped for a
     *     symlink pointing at a root-owned location after the initial realpath(),
     *     the tenant-credential write fails with EACCES because the tenant has
     *     no write permission there. O_CREAT|O_EXCL ('xb') additionally protects
     *     the final component. No lchown()/lchgrp() is required because the file
     *     is created directly as the install owner.
     *
     *     The uid/gid to drop to are read via lstat() (no final-symlink
     *     following) so a tenant who replaces $this->path with a symlink
     *     pointing at another tenant's directory gets that link's inode uid/gid,
     *     not the target's uid/gid (cross-tenant impersonation).  Additionally,
     *     if lstat reveals the install path is now a symlink, execution fails
     *     closed before any privilege manipulation.
     *
     *   When called as the install owner ($runningAsUser === true):
     *     Following an owner-planted symlink crosses no trust boundary (root is
     *     not involved), so no privilege manipulation is needed.
     *
     * @since 1.17.10 — root-context path drops to install-owner
     *     credentials (replaces path-based lchown/lchgrp, closes
     *     intermediate-component TOCTOU that O_EXCL alone cannot address);
     *     uid/gid read via lstat() to prevent cross-tenant impersonation via
     *     a final-component symlink swap.
     *
     * @param bool $runningAsUser
     *
     * @return bool  True when the flag file was created or already present;
     *     false when the write failed or was refused (including Layer 2
     *     expected-owner mismatch, which logs and returns false without
     *     aborting the batch).
     *
     * @throws LSCMException  Thrown when unlink() call on preexisting flag file
     *     fails.
     * @throws LSCMException  Thrown when posix extension is unavailable in root
     *     context (fail-closed; no acceptable fallback exists).
     * @throws LSCMException  Thrown when any component of the install path is
     *     a symlink (fail-closed; cross-tenant TOCTOU attack detected).
     * @throws LSCMException  Thrown when the install path is no longer
     *     accessible (fail-closed; possible TOCTOU attack).
     * @throws LSCMException  Thrown when the install path has been replaced by
     *     a symlink since the scan snapshot (fail-closed; TOCTOU attack
     *     detected).
     * @throws LSCMException  Thrown when the install path no longer
     *     canonicalises to the same value as the scan-time snapshot
     *     (fail-closed; TOCTOU attack detected).
     * @throws LSCMException  Thrown when the install directory is owned by root
     *     (fail-closed; privilege drop to root is not a privilege drop).
     * @throws LSCMException  Thrown indirectly by
     *     $this->restorePrivileges() call when credential restore fails
     *     (process state is undefined; thrown from the finally block).
     * @throws LSCMException  Thrown indirectly by Context::getFlagFileContent()
     *     call.
     */
    public function addUserFlagFile( $runningAsUser = true )
    {
        $flagFile = "$this->path/" . self::FLAG_FILE;

        if ( !$runningAsUser ) {
            /**
             * V8 — root context: drop to install-owner credentials for both the
             * unlink and the create so that intermediate-directory symlink swaps
             * redirecting the path off-tree fail with EACCES rather than
             * succeeding silently.  O_CREAT|O_EXCL ('xb') still guards the final
             * component.  Privileges are restored in the finally block so an
             * exception cannot leave the process running as the tenant.
             *
             * V9 — three additional layers close the residual cross-tenant
             * intermediate-component swap gap:
             *   1. assertNoSymlinkComponents(): per-component lstat() walk that
             *      refuses at the first symlink found in any directory component
             *      of $this->path, closing the gap before any uid/gid read.
             *   2. realpath() re-verification: $this->path was set to the
             *      canonical value at construct time; we re-canonicalise and
             *      require equality.  An intermediate swap that changes the
             *      canonical resolution is caught here even if Layer 1 misses it.
             *   3. Expected-owner check: the uid/gid reported by lstat() at
             *      write time is compared against the panel-assigned docroot
             *      owner captured at scan time.  A swap that redirects the path
             *      to another tenant's tree produces a mismatched uid even if
             *      Layers 1 and 2 fail to fire.
             *
             * V9.8 — validation logic extracted into resolveValidatedFlagOwnerUidGid()
             *      so that the identical trust-boundary checks are shared with the
             *      root-context removeFlagFile(false) path and cannot drift.
             */
            $owner = $this->resolveValidatedFlagOwnerUidGid('flag-file write');

            if ( $owner === false ) {
                return false;
            }

            list($uid, $gid) = $owner;

            $origEuid = posix_geteuid();
            $origEgid = posix_getegid();

            try {
                $this->dropPrivileges($uid, $gid);

                if ( !$this->writeFlagFileExclusive($flagFile) ) {
                    return false;
                }
            }
            finally {
                $this->restorePrivileges($origEuid, $origEgid);
            }
        }
        else {
            /**
             * Running as the install owner — no privilege boundary to enforce.
             */
            if ( !$this->writeFlagFileExclusive($flagFile) ) {
                return false;
            }
        }

        $this->setStatusBit(self::ST_FLAGGED);
        return true;
    }

    /**
     * Shared root-context path and owner validation for flag-file mutation.
     *
     * Performs, in order:
     *   1. POSIX availability check.
     *   2. Per-component lstat() walk (V9 Layer 1).
     *   3. Final lstat($this->path) with inaccessibility and symlink checks.
     *   4. realpath() re-verification (V9 Layer 3).
     *   5. Expected-owner comparison when bound (V9 Layer 2).
     *   6. Root-owner refusal.
     *
     * Three-way return contract (intentional — do not collapse into two):
     *   - Hard failure (symlink component, inaccessible path, canonical drift,
     *     root owner, posix unavailable) → throw LSCMException.  The caller
     *     must not catch and continue; the operation is aborted.
     *   - Expected-owner mismatch (V9 Layer 2) → return false.  The mismatch
     *     is already logged via Logger::error() here so the caller just
     *     propagates false without re-logging.  This is a per-install soft
     *     failure so a batch can continue to the next install.  Do NOT turn
     *     this into a throw without auditing every batch call site.
     *   - Success → return array($uid, $gid) for caller to pass to
     *     dropPrivileges().
     *
     * @since 1.17.10
     *
     * @param string $context  Human-readable label (e.g. 'flag-file write')
     *     used in exception and log messages.
     *
     * @return array|false  array($uid, $gid) on success; false on expected-
     *     owner mismatch (already logged).
     *
     * @throws LSCMException  When posix functions are unavailable.
     * @throws LSCMException  When any path component is a symlink.
     * @throws LSCMException  When the install path is inaccessible.
     * @throws LSCMException  When the install path is a symlink.
     * @throws LSCMException  When realpath() diverges from the stored path.
     * @throws LSCMException  When the install directory is root-owned.
     */
    private function resolveValidatedFlagOwnerUidGid( $context )
    {
        $this->checkPosixAvailability($context);
        $this->assertNoSymlinkComponents();

        clearstatcache(true, $this->path);
        $lstat = lstat($this->path);

        if ( $lstat === false ) {
            throw new LSCMException(
                "Refusing $context: install path is no longer accessible "
                    . '(possible TOCTOU attack).'
            );
        }

        if ( is_link($this->path) ) {
            throw new LSCMException(
                "Refusing $context: install path has been replaced with a "
                    . 'symlink since the scan snapshot (TOCTOU attack detected).'
            );
        }

        $recanon = realpath($this->path);

        if ( $recanon === false || $recanon !== $this->path ) {
            throw new LSCMException(
                "Refusing $context: install path no longer canonicalises to "
                    . 'the stored snapshot (TOCTOU attack detected).'
            );
        }

        $uid = (int)$lstat['uid'];
        $gid = (int)$lstat['gid'];

        if (
                $this->expectedOwnerUid !== null
                &&
                (
                    $uid !== $this->expectedOwnerUid
                    || $gid !== $this->expectedOwnerGid
                )
        ) {
            /**
             * Soft fail-closed: log and return false so the batch can continue
             * to the next install.  The security guarantee is identical to a
             * throw (the mutation is refused), but a throw would abort the
             * entire batch rather than just this install.
             */
            Logger::error(
                "$this->path - Refusing $context: install directory owner "
                    . "(uid=$uid gid=$gid) does not match the expected owner "
                    . 'captured at scan time '
                    . "(uid={$this->expectedOwnerUid} "
                    . "gid={$this->expectedOwnerGid}); "
                    . 'possible cross-tenant TOCTOU attack.'
            );

            return false;
        }

        if ( $uid === 0 || $gid === 0 ) {
            throw new LSCMException(
                "Refusing $context: install directory owner resolves to root "
                    . '(uid=0 or gid=0); privilege drop would be a no-op.'
            );
        }

        return array($uid, $gid);
    }

    /**
     * Walk every directory component of $this->path top-down using lstat() and
     * throw if any component is a symlink.  Because the walk refuses at the
     * first symlink found, no later lstat() call in the walk can be misled by
     * an earlier swapped component (lstat() still resolves intermediate
     * components before the final stat).
     *
     * This is the closest emulation of openat(AT_SYMLINK_NOFOLLOW) available
     * in pure PHP: it catches intermediate-directory symlink swaps (e.g.
     * tenant replaces public_html with a symlink into another user's tree)
     * before any uid/gid read is performed.
     *
     * @since 1.17.10
     *
     * @throws LSCMException  When $this->path is not an absolute path.
     * @throws LSCMException  When any path component cannot be stat'd.
     * @throws LSCMException  When any path component is a symlink.
     */
    private function assertNoSymlinkComponents()
    {
        if ( $this->path === '' || $this->path[0] !== '/' ) {
            throw new LSCMException(
                'Refusing flag-file write: install path is not absolute '
                    . '(possible TOCTOU attack).'
            );
        }

        $parts  = explode('/', $this->path);
        $prefix = '';

        foreach ( $parts as $part ) {
            if ( $part === '' ) {
                continue;
            }

            $prefix .= "/$part";

            clearstatcache(true, $prefix);

            if ( lstat($prefix) === false ) {
                throw new LSCMException(
                    "Refusing flag-file write: path component $prefix is no "
                        . 'longer accessible (possible TOCTOU attack).'
                );
            }

            if ( is_link($prefix) ) {
                throw new LSCMException(
                    "Refusing flag-file write: path component $prefix is a "
                        . 'symlink (cross-tenant TOCTOU attack detected).'
                );
            }
        }
    }

    /**
     * Fail-closed guard: throw if any posix function required for privilege
     * drop/restore is not available in this PHP build.
     *
     * @since 1.17.10
     *
     * @param string $context  Human-readable label used in the exception message.
     *
     * @throws LSCMException
     */
    private function checkPosixAvailability( $context )
    {
        $required = array(
            'posix_seteuid',
            'posix_setegid',
            'posix_geteuid',
            'posix_getegid',
            'posix_initgroups',
            'posix_getpwuid',
        );

        foreach ( $required as $fn ) {
            if ( !function_exists($fn) ) {
                throw new LSCMException(
                    "posix extension (function $fn) required for "
                        . "privilege-drop $context is not available."
                );
            }
        }
    }

    /**
     * Remove any preexisting flag file node (symlink or regular), then create a
     * new one with O_CREAT|O_EXCL ('xb') semantics and write the flag content.
     *
     * Called under whatever effective credentials the caller has arranged.
     * is_link() catches dangling symlinks that file_exists() reports as absent.
     *
     * @since 1.17.10
     *
     * @param string $flagFile  Absolute path to the flag file.
     *
     * @return bool  False when fopen or fwrite fails (caller should propagate).
     *
     * @throws LSCMException  When unlink() of a preexisting node fails.
     * @throws LSCMException  Thrown indirectly by Context::getFlagFileContent()
     *     call.
     */
    private function writeFlagFileExclusive( $flagFile )
    {
        if ( is_link($flagFile) || file_exists($flagFile) ) {
            if ( !unlink($flagFile) ) {
                throw new LSCMException(
                    'Failed to remove preexisting untrusted flag file.'
                );
            }
        }

        $flagFileContent = Context::getFlagFileContent();

        $fh = @fopen($flagFile, 'xb');

        if ( $fh === false ) {
            return false;
        }

        if ( fwrite($fh, $flagFileContent) === false ) {
            fclose($fh);
            @unlink($flagFile);
            return false;
        }

        fclose($fh);
        return true;
    }

    /**
     * Remove any flag file node (symlink or regular file) under whatever
     * effective credentials the caller has arranged.
     *
     * is_link() catches dangling symlinks that file_exists() reports as absent.
     * Mirrors the node-removal preamble of writeFlagFileExclusive() so that
     * the root-context removal path uses the same defensive node-type checks as
     * the creation path.
     *
     * @since 1.17.10
     *
     * @param string $flagFile  Absolute path to the flag file.
     *
     * @return bool  False when unlink() fails; true when the node was absent or
     *     successfully removed.
     */
    private function removeFlagFileNode( $flagFile )
    {
        if ( is_link($flagFile) || file_exists($flagFile) ) {
            if ( !unlink($flagFile) ) {
                return false;
            }
        }

        return true;
    }

    /**
     * Drop process credentials to ($uid, $gid) for a scoped root-context
     * operation inside an untrusted tenant directory.
     *
     * Order: posix_initgroups() → posix_setegid() → posix_seteuid().
     * initgroups() must precede seteuid() because only root can replace
     * supplementary groups; setegid() must precede seteuid() because once
     * euid is non-root the process cannot change its gid.
     * Non-root callers skip the initgroups step (only root holds dangerous
     * supplementary groups, and only root has CAP_SETGID to call initgroups).
     *
     * @since 1.17.10
     *
     * @param int $uid  Tenant effective uid.
     * @param int $gid  Tenant effective gid.
     *
     * @throws LSCMException  Thrown when uid cannot be resolved to a username
     *     for the supplementary-group reset (only in root context).
     * @throws LSCMException  Thrown when posix_initgroups() fails to replace
     *     the supplementary group set (only in root context).
     * @throws LSCMException  Thrown when posix_setegid()/posix_seteuid() fail
     *     to lower the effective gid/uid.
     */
    private function dropPrivileges( $uid, $gid )
    {
        if ( posix_geteuid() === 0 ) {
            $pw = posix_getpwuid($uid);

            if ( $pw === false ) {
                throw new LSCMException(
                    "Failed to look up username for uid $uid during privilege drop."
                );
            }

            if ( !posix_initgroups($pw['name'], $gid) ) {
                throw new LSCMException(
                    'Failed to replace supplementary groups during privilege drop.'
                );
            }
        }

        if ( !posix_setegid($gid) || !posix_seteuid($uid) ) {
            throw new LSCMException(
                'Failed to drop effective uid/gid during privilege drop.'
            );
        }
    }

    /**
     * Restore process credentials to root after a dropPrivileges() scope.
     * Must always be called inside a finally block.
     *
     * Restore order: seteuid(root) → setegid(root) → initgroups('root').
     *
     * posix_seteuid(0) can only fail if the real uid is not 0 — i.e., if this
     * method is called outside of a root process, which is a programming error.
     * Throwing here is correct: the calling process is in an undefined state
     * and must not continue operating as the tenant.
     *
     * @since 1.17.10
     *
     * @param int $origEuid  Saved euid from before dropPrivileges().
     * @param int $origEgid  Saved egid from before dropPrivileges().
     *
     * @throws LSCMException  Thrown when posix_seteuid()/posix_setegid() fail
     *     to restore effective uid/gid (indicates process is in an undefined
     *     state and must not continue).
     * @throws LSCMException  Thrown when posix_initgroups() fails to restore
     *     the supplementary group set (indicates process is in an undefined
     *     state and must not continue).
     */
    private function restorePrivileges( $origEuid, $origEgid )
    {
        if ( !posix_seteuid($origEuid) || !posix_setegid($origEgid) ) {
            throw new LSCMException(
                'Failed to restore effective uid/gid after privilege drop; '
                    . 'process state is undefined.'
            );
        }

        if ( $origEuid === 0 ) {
            if ( !posix_initgroups('root', $origEgid) ) {
                throw new LSCMException(
                    'Failed to restore supplementary groups after privilege drop; '
                        . 'process state is undefined.'
                );
            }
        }
    }

    /**
     * Remove the flag file from this install.
     *
     * When $runningAsUser is false (root context) the same V9 trust-boundary
     * checks as addUserFlagFile(false) are applied via
     * resolveValidatedFlagOwnerUidGid() before dropping privileges for the
     * removal: per-component lstat() walk, realpath() re-verification,
     * expected-owner comparison, and root-owner refusal.  This closes the
     * cross-tenant TOCTOU gap for the deletion path that was present before
     * V9.8.
     *
     * @since 1.17.10 — root-context path ($runningAsUser=false)
     *     applies the same V9 owner-binding and symlink-walk guards as
     *     addUserFlagFile(false), then drops to install-owner credentials for
     *     the unlink.
     *
     * @param bool $runningAsUser
     *
     * @return bool  True when the flag file was absent or successfully removed;
     *     false when the removal failed or was refused (including expected-owner
     *     mismatch, which logs and returns false without aborting the batch).
     *
     * @throws LSCMException  When posix extension is unavailable in root context.
     * @throws LSCMException  When any component of the install path is a symlink.
     * @throws LSCMException  When the install path is no longer accessible.
     * @throws LSCMException  When the install path has been replaced by a symlink.
     * @throws LSCMException  When realpath() diverges from the stored snapshot.
     * @throws LSCMException  When the install directory is owned by root.
     * @throws LSCMException  Thrown indirectly by restorePrivileges() when
     *     credential restore fails (process state is undefined; thrown from
     *     the finally block).
     */
    public function removeFlagFile( $runningAsUser = true )
    {
        $flagFile = "$this->path/" . self::FLAG_FILE;

        if ( !$runningAsUser ) {
            $owner = $this->resolveValidatedFlagOwnerUidGid('flag-file removal');

            if ( $owner === false ) {
                return false;
            }

            list($uid, $gid) = $owner;

            $origEuid = posix_geteuid();
            $origEgid = posix_getegid();

            try {
                $this->dropPrivileges($uid, $gid);

                if ( !$this->removeFlagFileNode($flagFile) ) {
                    return false;
                }
            }
            finally {
                $this->restorePrivileges($origEuid, $origEgid);
            }
        }
        else {
            if ( file_exists($flagFile) ) {

                if ( !unlink($flagFile) ) {
                    return false;
                }
            }
        }

        $this->unsetStatusBit(self::ST_FLAGGED);
        return true;
    }

    /**
     * Add a flag file to indicate that a new LSCWP plugin was added to this
     * installation.
     *
     * This function should only be called by the installation owner to avoid
     * permission problems involving this file.
     *
     * @return bool
     */
    public function addNewLscwpFlagFile()
    {
        $file = "$this->path/" . self::FLAG_NEW_LSCWP;

        if ( !file_exists($file) ) {

            if ( !file_put_contents($file, '') ) {
                return false;
            }
        }

        return true;
    }

    /**
     * Remove "In Progress" flag file to indicate that a WPInstall action has
     * been completed.
     *
     * @return bool
     */
    public function removeNewLscwpFlagFile()
    {
        $file = "$this->path/" . self::FLAG_NEW_LSCWP;

        if ( file_exists($file) ) {

            if ( !unlink($file) ) {
                return false;
            }
        }

        return true;
    }

    /**
     *
     * @param bool $forced
     *
     * @return int
     *
     * @throws LSCMException  Thrown indirectly by UserCommand::issue() call.
     */
    public function refreshStatus( $forced = false )
    {
        if ( !$this->refreshed || $forced ) {
            UserCommand::issue(UserCommand::CMD_STATUS, $this);
            $this->refreshed = true;
        }

        return $this->getData(self::FLD_STATUS);
    }

    /**
     *
     * @param string $pluginDir
     *
     * @throws LSCMException  Thrown indirectly by Logger::debug() call.
     */
    public function removePluginFiles( $pluginDir )
    {
        if ( !is_string($pluginDir)
                || $pluginDir === ''
                || $pluginDir === '/'
                || $pluginDir[0] !== '/'
                || in_array('..', explode('/', $pluginDir), true) ) {

            Logger::debug(
                "$this->path - Refusing to remove plugin files: unsafe path."
            );
            return;
        }

        if ( !file_exists($pluginDir) && !is_link($pluginDir) ) {
            return;
        }

        if ( is_link($pluginDir) ) {
            /**
             * Plugin directory is a symlink. Remove the symlink node only; do
             * NOT resolve it via realpath() + rm -rf, which would recursively
             * delete the target directory (potentially outside wp-content/).
             */
            if ( @unlink($pluginDir) ) {
                Logger::debug(
                    "$this->path - Removed LSCache for WordPress plugin "
                        . 'symlink from plugins directory'
                );
            }
            else {
                Logger::debug(
                    "$this->path - Failed to remove LSCache for WordPress "
                        . 'plugin symlink from plugins directory'
                );
            }

            return;
        }

        exec('rm -rf ' . escapeshellarg($pluginDir));

        Logger::debug(
            "$this->path - Removed LSCache for WordPress plugin files from "
                . 'plugins directory'
        );
    }

    /**
     *
     * @return bool
     */
    public function isFlagBitSet()
    {
        if ( ($this->getStatus() & self::ST_FLAGGED) ) {
            return true;
        }

        return false;
    }

    /**
     *
     * @param null|int $status
     *
     * @return bool
     */
    public function hasFatalError( $status = null )
    {
        if ( $status === null ) {
            $status = $this->getData(self::FLD_STATUS);
        }

        $errMask = (
            self::ST_ERR_EXECMD
                | self::ST_ERR_EXECMD_DB
                | self::ST_ERR_TIMEOUT
                | self::ST_ERR_SITEURL
                | self::ST_ERR_DOCROOT
                | self::ST_ERR_WPCONFIG
        );

        return (($status & $errMask) > 0);
    }

    /**
     *
     * @since 1.17.10
     *
     * @return PhpBinaryParts
     *
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance() call.
     * @throws LSCMException  Thrown indirectly by
     *     ControlPanel::getClassInstance()->getPhpBinaryParts() call.
     */
    public function getPhpBinaryParts()
    {
        if ( $this->phpBinaryParts === null ) {
            $this->phpBinaryParts =
                ControlPanel::getClassInstance()->getPhpBinaryParts($this);
        }

        return $this->phpBinaryParts;
    }

    /**
     * @deprecated since 1.17.10  Call getPhpBinaryParts() instead.
     *
     * @return string
     *
     * @throws LSCMException  Thrown indirectly by getPhpBinaryParts() call.
     */
    public function getPhpBinary()
    {
        if ( $this->phpBinary === null ) {
            $parts         = $this->getPhpBinaryParts();
            $options       = $parts->getOptionsString();
            $this->phpBinary = $options === ''
                ? $parts->getBinPath()
                : $parts->getBinPath() . ' ' . $options;
        }

        return $this->phpBinary;
    }

    /**
     * Returns requested owner info.
     *
     * @param string $field  Key ('user_id', 'user_name', or 'group_id') in the
     *     $ownerInfo array.
     *
     * @return array|int|string|null
     */
    public function getOwnerInfo( $field = '' )
    {
        if ( $this->ownerInfo == null ) {
            $this->ownerInfo = Util::populateOwnerInfo($this->path);
        }

        if ( $field == '' ) {
            return $this->ownerInfo;
        }
        elseif ( isset($this->ownerInfo[$field]) ) {
            return $this->ownerInfo[$field];
        }

        return null;
    }

    /**
     *
     * @return string
     */
    public function getSuCmd()
    {
        if ( $this->suCmd == null ) {
            $userName    = (string)$this->getOwnerInfo('user_name');
            $this->suCmd = 'su ' . escapeshellarg($userName) . ' -s /bin/bash';
        }

        return $this->suCmd;
    }

    /**
     *
     * @return int
     */
    public function getCmdStatus()
    {
        return $this->cmdStatus;
    }

    /**
     *
     * @return string
     */
    public function getCmdMsg()
    {
        return $this->cmdMsg;
    }

    public function setCmdStatusAndMsg( $status, $msg )
    {
        $this->cmdStatus = $status;
        $this->cmdMsg    = $msg;
    }

}
