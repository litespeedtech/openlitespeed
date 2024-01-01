<?php

/** *********************************************
 * LiteSpeed Web Server WordPress Dash Notifier
 *
 * @author    Michael Alegre
 * @copyright 2019-2023 LiteSpeed Technologies, Inc.
 * *******************************************
 */

namespace Lsc\Wp;

class WPDashMsgs
{

    /**
     * @var string
     */
    const MSG_TYPE_RAP = 'rap';

    /**
     * @var string
     */
    const MSG_TYPE_BAM = 'bam';

    /**
     * @var string
     */
    const KEY_RAP_MSGS = 'rapMsgs';

    /**
     * @var string
     */
    const KEY_BAM_MSGS = 'bamMsgs';

    /**
     * Do not change the following constant values, substr 'msg' is used in
     * PanelController to determine action.
     */

    /**
     * @var string
     */
    const ACTION_GET_MSG = 'msgGet';

    /**
     * @var string
     */
    const ACTION_ADD_MSG = 'msgAdd';

    /**
     * @var string
     */
    const ACTION_DELETE_MSG = 'msgDelete';

    /**
     * @var string
     */
    protected $dataFile;

    /**
     * @var string[][]
     */
    protected $msgData = array();


    public function __construct( )
    {
        $this->dataFile =
            realpath(__DIR__ . '/../../..') . '/admin/lscdata/wpDashMsgs.data';

        $this->init();
    }

    protected function init()
    {
        if ( file_exists($this->dataFile) ) {
            $data = json_decode(file_get_contents($this->dataFile), true);

            if ( $data && is_array($data) ) {
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
        $this->msgData[self::KEY_RAP_MSGS] = array_merge(
            array(
                'default' => array(
                    'msg'  => 'Greetings! This is your hosting company '
                        . 'encouraging you to click the button to install the '
                        . 'LiteSpeed Cache plugin. This plugin will speed up '
                        . 'your WordPress site dramatically. Please contact us '
                        . 'with any questions.',
                    'slug' => 'litespeed-cache'
                )
            ),
            $this->msgData[self::KEY_RAP_MSGS]
        );
    }

    /**
     *
     * @param string $type
     *
     * @return string[]|string[][]
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
     *
     * @return bool
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
     *
     * @return bool
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
        file_put_contents($this->dataFile, json_encode($this->msgData));
    }

}
