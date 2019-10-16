<?php

/**
 * Expects: $d['name'], $d['value']
 * Optional: $d['class'], $d['title'], $d['confirm'], $d['onclick'],
 *           $d['attributes'], $d['state'], and $d['text']
 */

?>

<button type="submit" name="<?php echo $d['name']; ?>"
        value="<?php echo $d['value']; ?>"

        <?php if ( isset($d['class']) ) : ?>

        class="<?php echo $d['class']; ?>"

        <?php

        endif;

        if ( isset($d['title']) ):

        ?>

        title="<?php echo $d['title']; ?>"

        <?php

        endif;

        if ( isset($d['confirm']) ) :

        ?>

        onclick="return confirm('<?php echo $d['confirm']; ?>')"

        <?php elseif ( isset($d['onclick']) ): ?>

        onclick="return <?php echo $d['onclick']; ?>"

        <?php

        endif;

        echo (isset($d['attributes'])) ? " {$d['attributes']}" : '';

        echo (isset($d['state'])) ? " {$d['state']}" : '';

        ?>
    >
  <?php echo (isset($d['text'])) ? $d['text'] : $d['value']; ?>
</button>
