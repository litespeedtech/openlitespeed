<?php

/* * *********************************************
 * LiteSpeed Web Server WordPress Dash Notifier
 *
 * @author LiteSpeed Technologies, Inc. (https://www.litespeedtech.com)
 * @copyright (c) 2019
 * *******************************************
 */

namespace Lsc\Wp;

class WPDashMsgs
{

    const MSG_TYPE_RAP = 'rap';
    const MSG_TYPE_BAM = 'bam';
    const KEY_RAP_MSGS = 'rapMsgs';
    const KEY_BAM_MSGS = 'bamMsgs';

    /**
     * Do not change these values, substr 'msg' is used in PanelController to
     * determine action.
     */
    const ACTION_GET_MSG = 'msgGet';
    const ACTION_ADD_MSG = 'msgAdd';
    const ACTION_DELETE_MSG = 'msgDelete';

    /**
     * @var string
     */
    protected $dataFile;

    /**
     * @var string[][]
     */
    protected $msgData = array();


    /**
     *
     * @param string  $dataFile
     */
    public function __construct( )
    {
        $this->dataFile = LSWS_HOME . '/admin/lscdata/wpDashMsgs.data';

        $this->init();
    }

    protected function init()
    {
        if ( file_exists($this->dataFile) ) {
            $data = json_decode(file_get_contents($this->dataFile), true);

            if ( $data != false && is_array($data) ) {
                $this->msgData = $data;
            }
        }

        if ( !isset($this->msgData[self::KEY_RAP_MSGS]) ) {
            $this->msgData[self::KEY_RAP_MSGS] = array();
        }

        if ( !isset($this->msgData[self::KEY_BAM_MSGS]) ) {
            $this->msgData[self::KEY_BAM_MSGS] = array();
        }

        /**
         * Set default rap message and plugin slug.
         */
        $defaultRap = array(
            'default' => array(
                'msg' => 'Greetings! This is your hosting company encouraging you to click the button '
                    . 'to install the LiteSpeed Cache plugin. This plugin will speed up your '
                    . 'WordPress site dramatically. Please contact us with any questions.',
                'slug' => 'litespeed-cache')
        );

        $this->msgData[self::KEY_RAP_MSGS] =
                array_merge($defaultRap, $this->msgData[self::KEY_RAP_MSGS]);
    }

    /**
     *
     * @param string  $type
     * @return sting[]|string[][]
     */
    public function getMsgData( $type = '' )
    {
        switch ($type) {
            case self::MSG_TYPE_RAP:
                return $this->msgData[self::KEY_RAP_MSGS];

            case self::MSG_TYPE_BAM:
                return $this->msgData[self::KEY_BAM_MSGS];

            default:
                return $this->msgData;
        }
    }

    /**
     *
     * @param string $type
     * @param string $msgId
     * @param string $msg
     * @param string $slug
     * @return boolean
     */
    public function addMsg( $type, $msgId, $msg, $slug = '' )
    {
        if ( $msgId === ''
                || $msgId === NULL
                || ($msgId == 'default' && $type == self::MSG_TYPE_RAP)
                || strlen($msgId) > 50
                || preg_match('/[^a-zA-Z0-9_-]/', $msgId) ) {

            return false;
        }

        switch ($type) {
            case self::MSG_TYPE_RAP:
                $this->msgData[self::KEY_RAP_MSGS][$msgId] =
                        array( 'msg' => $msg, 'slug' => $slug );
                break;
            case self::MSG_TYPE_BAM:
                $this->msgData[self::KEY_BAM_MSGS][$msgId] =
                        array( 'msg' => $msg );
                break;
            default:
                return false;
        }

        $this->saveDataFile();
        return true;
    }

    /**
     *
     * @param string $type
     * @param string $msgId
     * @return boolean
     */
    public function deleteMsg( $type, $msgId )
    {
        if ( $msgId === '' || $msgId === NULL ) {
            return false;
        }

        switch ($type) {
            case self::MSG_TYPE_RAP:

                if ( $msgId == 'default' ) {
                    return false;
                }

                $key = self::KEY_RAP_MSGS;
                break;
            case self::MSG_TYPE_BAM:
                $key = self::KEY_BAM_MSGS;
                break;
            default:
                return false;
        }

        if ( isset($this->msgData[$key][$msgId]) ) {
            unset($this->msgData[$key][$msgId]);

            $this->saveDataFile();
            return true;
        }

        return false;
    }

    protected function saveDataFile()
    {
        $jsonData = json_encode($this->msgData);

        file_put_contents($this->dataFile, $jsonData);
    }

}
