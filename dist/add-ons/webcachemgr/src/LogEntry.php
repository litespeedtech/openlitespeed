<?php

/** *********************************************
 * LiteSpeed Web Server Cache Manager
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2018-2019
 * *******************************************
 */

namespace Lsc\Wp;

class LogEntry
{

    /**
     * @var int
     */
    protected $lvl;

    /**
     * @var string
     */
    protected $msg;

    /**
     *
     * @var null|string
     */
    protected $prefix;

    /**
     *
     * @var int[]
     */
    protected $timestamp;

    /**
     *
     * @param string  $msg
     * @param int     $lvl
     */
    public function __construct( $msg, $lvl )
    {
        $this->msg = $msg;
        $this->lvl = $lvl;
        $this->timestamp = time();
    }

    /**
     *
     * @param string  $msg
     */
    public function append( $msg )
    {
        $this->msg .= $msg;
    }

    /**
     *
     * @return int
     */
    public function getLvl()
    {
        return $this->lvl;
    }

    /**
     *
     * @return string
     */
    public function getMsg()
    {
        $msg = ($this->prefix == null) ? '' : "{$this->prefix} ";

        if ( $this->msg ) {
            $msg .= $this->msg;
        }

        return $msg;
    }

    /**
     *
     * @param int  $logLvl
     * @return string
     * @throws LSCMException  Thrown indirectly.
     */
    public function getOutput( $logLvl )
    {
        if ( $this->lvl > $logLvl ) {
            return '';
        }


        $timestamp = date('Y-m-d H:i:s', $this->timestamp);
        $addTagInfo = Logger::getAdditionalTagInfo();
        $lvl = '[' . Logger::getLvlDescr($this->lvl) . ']';
        $msg = $this->getMsg();

        return "{$timestamp} {$addTagInfo} {$lvl}  {$msg}\n";
    }

}
