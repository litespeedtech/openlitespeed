<?php

$d = array(
    'title' => 'Cache Root Not Set',
);
$this->loadTplBlock('Title.tpl', $d);

?>

<div>
  <p>
    Server and/or Virtual Host cache root has not yet been set. Please go to
    <a href="?do=cacheRootSetup" title="Go to Cache Root Setup">Cache Root Setup</a>
    and set these first.
  </p>
</div>

<?php

$d = array(
    'back' => 'Back'
);
$this->loadTplBlock('ButtonPanelBackNext.tpl', $d);
