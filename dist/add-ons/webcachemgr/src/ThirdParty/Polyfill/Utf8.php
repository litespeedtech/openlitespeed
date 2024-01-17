<?php

/**
 * This file contains paired-down/modified code originally found in Symfony
 * package file Php72.php, used here as a PHP standard replacement for now
 * deprecated PHP functions utf8_encode() and utf8_decode().
 *
 * Modified by: Michael Alegre (LiteSpeed Technologies, Inc.), 2023
 *
 * (c) Fabien Potencier <fabien@symfony.com>
 *
 * For the full copyright and license information, please view the LICENSE
 * file that was distributed with this source code.
 */

namespace Lsc\Wp\ThirdParty\Polyfill;

/**
 * @since 1.16.1
 */
class Utf8
{
    /**
     *
     * @since 1.16.1
     *
     * @param $s
     *
     * @return false|string
     */
    public static function encode($s)
    {
        $s .= $s;
        $len = strlen($s);

        for ($i = $len >> 1, $j = 0; $i < $len; ++$i, ++$j) {

            if ( $s[$i] < "\x80" ) {
                $s[$j] = $s[$i];
            }
            elseif ( $s[$i] < "\xC0" ) {
                $s[$j] = "\xC2";
                $s[++$j] = $s[$i];
            }
            else {
                $s[$j] = "\xC3";
                $s[++$j] = chr(ord($s[$i]) - 64);
            }
        }

        return substr($s, 0, $j);
    }

    /**
     *
     * @since 1.16.1
     *
     * @param $s
     *
     * @return false|string
     */
    public static function decode($s)
    {
        $s = (string) $s;
        $len = strlen($s);

        for ($i = 0, $j = 0; $i < $len; ++$i, ++$j) {

            switch ($s[$i] & "\xF0") {
                case "\xC0":
                case "\xD0":
                    $c = (ord($s[$i] & "\x1F") << 6) | ord($s[++$i] & "\x3F");
                    $s[$j] = $c < 256 ? chr($c) : '?';
                    break;

                case "\xF0":
                    ++$i;
                // fallthrough

                case "\xE0":
                    $s[$j] = '?';
                    $i += 2;
                    break;

                default:
                    $s[$j] = $s[$i];
            }
        }

        return substr($s, 0, $j);
    }

}