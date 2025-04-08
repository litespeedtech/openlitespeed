<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author    Michael Alegre
 * @copyright 2019-2025 LiteSpeed Technologies, Inc.
 * @since     1.9
 * *******************************************
 */

namespace Lsc\Wp;

/**
 * @since 1.9
 */
class AjaxResponse
{

    /**
     * @since 1.9
     * @var   AjaxResponse
     */
    protected static $instance;

    /**
     * @since 1.9
     * @var   string
     */
    protected $ajaxContent;

    /**
     * @since 1.9
     * @var   string
     */
    protected $headerContent;

    /**
     *
     * @since 1.9
     */
    private function __construct()
    {

    }

    /**
     *
     * @since 1.9
     *
     * @return AjaxResponse
     */
    private static function me()
    {
        if ( self::$instance == null ) {
            self::$instance = new self();
        }

        return self::$instance;
    }

    /**
     *
     * @since 1.9
     *
     * @throws LSCMException
     */
    public static function outputAndExit()
    {
        $output = '';

        if ( self::$instance == null ) {
            throw new LSCMException('AjaxResponse object never created!');
        }

        $m = self::me();

        if ( !empty($m->headerContent) ) {
            $output .= "{$m->headerContent}\n\n";
        }

        $output .= $m->ajaxContent;

        ob_clean();
        echo $output;
        exit;
    }

    /**
     *
     * @since 1.9
     *
     * @param string $ajaxContent
     */
    public static function setAjaxContent( $ajaxContent )
    {
        self::me()->ajaxContent = $ajaxContent;
    }

    /**
     *
     * @since 1.9
     *
     * @param string $headerContent
     */
    public static function setHeaderContent( $headerContent )
    {
        self::me()->headerContent = $headerContent;
    }

}
