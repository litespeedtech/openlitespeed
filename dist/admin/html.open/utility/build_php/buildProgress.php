<?php
require_once('../../includes/auth.php');

include_once( 'buildconf.inc.php' );

$progress_file = $_SESSION['progress_file'];
$log_file = $_SESSION['log_file'];

$progress = htmlspecialchars(file_get_contents($progress_file));
echo $progress;

echo "\n**LOG_DETAIL** retrieved from $log_file\n";
$log = htmlspecialchars(file_get_contents($log_file));
echo $log;
