<?php
$raw = $_SERVER['argv'][1];
$valid_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789/.";
if (CRYPT_MD5 == 1) 
{
    $salt = '$1$';
    for($i = 0; $i < 8; $i++) 
    {
        $salt .= $valid_chars[rand(0,strlen($valid_chars)-1)];
    }
    $salt .= '$';
}
else
{
    $salt = $valid_chars[rand(0,strlen($valid_chars)-1)];
    $salt .= $valid_chars[rand(0,strlen($valid_chars)-1)];
}
$encypt = crypt($raw, $salt);
echo "$encypt\n";
?>