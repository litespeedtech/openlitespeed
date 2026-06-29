<?php

/** ******************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2026 LiteSpeed Technologies, Inc.
 * @since 1.17.10
 * ******************************************* */

namespace Lsc\Wp\Panel;

/**
 * Value object returned by ControlPanel::getPhpBinaryParts().
 *
 * Separates the PHP binary path (one shell token, will be escapeshellarg'd
 * by the consumer) from the options string (shell-active, library-built).
 *
 * @since 1.17.10
 */
class PhpBinaryParts
{

    /**
     * @since 1.17.10
     * @var string
     */
    private $binPath;

    /**
     * @since 1.17.10
     * @var string
     */
    private $optionsString;

    /**
     *
     * @since 1.17.10
     *
     * @param string $binPath        Filesystem path to PHP executable.
     * @param string $optionsString  PHP CLI flags as a shell-active string.
     *                               May be empty.
     */
    public function __construct( $binPath, $optionsString = '' )
    {
        $this->binPath       = (string)$binPath;
        $this->optionsString = (string)$optionsString;
    }

    /**
     *
     * @since 1.17.10
     *
     * @return string
     */
    public function getBinPath()
    {
        return $this->binPath;
    }

    /**
     *
     * @since 1.17.10
     *
     * @return string
     */
    public function getOptionsString()
    {
        return $this->optionsString;
    }

}
