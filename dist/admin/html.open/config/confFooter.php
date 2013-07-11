
<?

echo '
<input type="hidden" name="a" value="s">
<input type="hidden" name="m" value="' . $disp->_mid 
. '"><input type="hidden" name="p" value="' . $disp->_pid
. '"><input type="hidden" name="t" value="' . $disp->_tid
. '"><input type="hidden" name="r" value="' . $disp->_ref
. '"><input type="hidden" name="tk" value="' . $client->token
. '"><input type="hidden" name="file_create" value="">
</div>
</form>';

echo GUI::footer();

?>

