<?php

if ($argc == 1) {
	echo "Usage: $argv[0] lang [mixed]
		lang is the language file you want to sort. Current supported values
            'english', 'chinese', 'japanes'
		If supply 2nd param mixed, the translated file will mix all the tags in the sorted order.
        Otherwise, the new tags will show up in the end\n\n";
	return;
}

include(__DIR__ . '/../../lib/DMsg.php');

$option = '';
if (isset($argv[2])) {
    $option = $argv[2];
}

DMsg::Util_SortMsg($argv[1], $option);

