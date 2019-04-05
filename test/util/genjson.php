<?php

if (defined('STDIN') && !is_null($argv[1])) {
  $depth = $argv[1];
} else {
  die("Usage: \nphp genjson.php <depth>\n");
}

$temp = array();
for($i = $depth; $i > 0; $i--) {
    $temp = array($temp);
}

echo json_encode($temp);
