<?php
// $Id: signup_edit_form.inc,v 1.1.2.6 2009/09/19 00:33:42 dww Exp $


/**
 * @file
 * Code for the form to edit existing signups.
 */

/**
 * Build the form for editing existing signups.
 *
 * @param $form_state
 *   The state of the form to build (not currently used).
 * @param $signup
 *   The fully-loaded signup object with data about the signup to edit.
 * @param $type
 *   The type of signup edit form to render, can be 'node', 'tab', or 'admin'.
 *
 * @return
 *   The FAPI form array for the signup edit form.
 */
function signup_edit_form($form_state, $signup, $type) {
  global $user;
  $form = array();

  $form['#signup'] = $signup;
  $node = node_load($signup->nid);

  // Check permissions.
  $admin = _signup_menu_access($node, 'admin');
  $own = !empty($user->uid) && $user->uid == $signup->uid;
  $can_cancel = $admin || (user_access('cancel own signups') && $own);
  $can_edit = $admin || (user_access('edit own signups') && $own);

  if ($type == 'admin') {
    $title = t("Information for !user's signup to !node", array('!user' => $signup->first_name .' '.$signup->last_name, '!node' => l($node->title, 'node/'. $node->nid)));
  }
  else {
    $title = t('Your signup information');
  }

  if ($type == 'node') {
    $form['elements'] = array(
      '#type' => 'fieldset',
      '#title' => $title,
      '#collapsible' => TRUE,
      '#collapsed' => variable_get('signup_fieldset_collapsed', 1),
    );
  }
  else {
    $form['elements'] = array();
    $form['elements']['header'] = array(
      '#type' => 'markup',
      '#value' => $title,
      '#prefix' => '<h4>',
      '#suffix' => '</h4>',
    );
  }

  if (!empty($signup->anon_mail)) {
    $form['elements']['signup_anon_mail'] = array(
      '#type' => 'textfield',
      '#title' => t('Email'),
      '#default_value' => $signup->anon_mail,
      '#size' => 40,
      '#maxlength' => 255,
      '#required' => TRUE,
    );
    $form['#validate'][] = 'signup_validate_anon_mail';
  }

  if ($admin) {
    $options = array();
    if (1|| !isset($signup->attended)) {
      $options[-1] = t('- Not recorded -');
    }
    $options[1] = theme('signup_attended_text', 1);
    $options[0] = theme('signup_attended_text', 0);
    $form['elements']['attended'] = array(
      '#type' => 'select',
      '#title' => t('Attendance'),
      '#default_value' => isset($signup->attended) ? $signup->attended : -1,
      '#options' => $options
    );
  }

  // Build the themed signup form for this site and include that.
  $site_form = theme('signup_user_form', $node);
  $form_data = unserialize($signup->form_data);

  // This is sort of a hack, but we don't support nested arrays for the custom
  // signup form anyway, so it works for now.  Obviously all this will change
  // with signup_fields and friends, but for now it works.
  foreach ($form_data as $key => $value) {
    if (!empty($site_form['signup_form_data'][$key])) {
      $site_form['signup_form_data'][$key]['#default_value'] = $value;
      if (!$can_edit) {
        // If they can't edit, mark all the fields as disabled.
        $site_form['signup_form_data'][$key]['#disabled'] = TRUE;
      }
    }
  }
$site_form['first_name']['#default_value'] = $signup->first_name;
$site_form['last_name']['#default_value'] = $signup->last_name;
$site_form['user_title']['#default_value'] = $signup->user_title;
$site_form['organization']['#default_value'] = $signup->organization;
$site_form['address']['#default_value'] = $signup->address;
$site_form['city']['#default_value'] = $signup->city;
$site_form['state']['#default_value'] = $signup->state;
$site_form['zip_code']['#default_value'] = $signup->zip_code;
$site_form['phone_number']['#default_value'] = $signup->phone_number;
$site_form['fax_number']['#default_value'] = $signup->fax_number;
$site_form['mail_list']['#default_value'] = $signup->mail_list;
$site_form['CRC_credit']['#default_value'] = $signup->CRC_credit;
$site_form['accommodation_needs_if_any']['#default_value'] = $signup->accommodation_needs_if_any;
$site_form['ASL']['#default_value'] = $signup->ASL;
$site_form['CART']['#default_value'] = $signup->CART;
$site_form['payment_method']['#default_value'] = $signup->payment_method;
$site_form['screenreader']['#default_value'] = $signup->screenreader;
  $form['elements'] += $site_form;


  // Add the appropriate buttons based on permissions.
  if ($can_edit) {
    $form['elements']['save'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
      '#submit' => array('signup_edit_form_save_submit'),
    );
  }
  if ($can_cancel) {
    if (isset($_REQUEST['destination'])) {
      $destination = drupal_get_destination();
    }
    else {
      // If there's no destination already set, redirect to the node.
      $destination = 'destination='. urlencode("node/$signup->nid");
    }
    $signup_token = signup_get_token($signup->sid, 'cancel');
    $form['elements']['cancel-signup'] = array(
      '#type' => 'markup',
      '#value' => l(t('Cancel signup'), "signup/cancel/$signup->sid/$signup_token", array('query' => $destination)),
    );
  }

  return $form;
}

/**
 * Validation callback when editing the anonymous email for an existing signup.
 */
function signup_validate_anon_mail($form, &$form_state) {
  $mail = $form_state['values']['signup_anon_mail'];
  if (!valid_email_address($mail)) {
    form_set_error('signup_anon_mail', t('The e-mail address %mail is not valid.', array('%mail' => $mail)));
  }
}

/**
 * Submit callback when saving changes to an existing signup.
 */
function signup_edit_form_save_submit($form, $form_state) {
  $signup = $form['#signup'];
  if (!empty($form_state['values']['signup_form_data'])) {
    $signup->form_data = $form_state['values']['signup_form_data'];
  }

  // If the form contains an e-mail address for an anonymous signup, save it.
  if (!empty($form_state['values']['signup_anon_mail'])) {
    $signup->anon_mail = $form_state['values']['signup_anon_mail'];
  } if (!empty($form_state['values']['first_name'])) {
    $signup->first_name = $form_state['values']['first_name'];
  } if (!empty($form_state['values']['last_name'])) {
    $signup->last_name = $form_state['values']['last_name'];
  } if (!empty($form_state['values']['user_title'])) {
    $signup->user_title = $form_state['values']['user_title'];
  } if (!empty($form_state['values']['organization'])) {
    $signup->organization = $form_state['values']['organization'];
  } if (!empty($form_state['values']['address'])) {
    $signup->address = $form_state['values']['address'];
  } if (!empty($form_state['values']['city'])) {
    $signup->city = $form_state['values']['city'];
  } if (!empty($form_state['values']['state'])) {
    $signup->state = $form_state['values']['state'];
  } if (!empty($form_state['values']['zip_code'])) {
    $signup->zip_code = $form_state['values']['zip_code'];
  } if (!empty($form_state['values']['phone_number'])) {
    $signup->phone_number = $form_state['values']['phone_number'];
  } if (!empty($form_state['values']['fax_number'])) {
    $signup->fax_number = $form_state['values']['fax_number'];
  } if (!empty($form_state['values']['mail_list'])) {
    $signup->mail_list = $form_state['values']['mail_list'];
  } if (!empty($form_state['values']['CRC_credit'])) {
    $signup->CRC_credit = $form_state['values']['CRC_credit'];
  } if (!empty($form_state['values']['accommodation_needs_if_any'])) {
    $signup->accommodation_needs_if_any = $form_state['values']['accommodation_needs_if_any'];
  } if (!empty($form_state['values']['ASL'])) {
    $signup->ASL = $form_state['values']['ASL'];
  } if (!empty($form_state['values']['CART'])) {
    $signup->CART = $form_state['values']['CART'];
  } if (!empty($form_state['values']['payment_method'])) {
    $signup->payment_method = $form_state['values']['payment_method'];
  } if (!empty($form_state['values']['screenreader'])) {
    $signup->screenreader = $form_state['values']['screenreader'];
  }

  // If the form contains attendance info, save it.
  if (isset($form_state['values']['attended'])) {
    if ($form_state['values']['attended'] == -1) {
      unset($signup->attended);
    }
    else {
      $signup->attended = $form_state['values']['attended'];
    }
  }

  // Invoke hook_signup_data_alter() to let other modules change this.
  drupal_alter('signup_data', $signup, $form_state['values']);

  // Update the signup in the {signup_log} table.
  signup_save_signup($signup);

  // Because drupal_write_record() doesn't gracefully handle columns that can
  // be NULL, if the attendence was cleared out by this edit, we need to
  // manually set the DB record to NULL here.
  if (!isset($signup->attended)) {
    db_query("UPDATE {signup_log} SET attended = NULL WHERE sid = %d", $signup->sid);
  }

  drupal_set_message(t('Signup information updated.'));
}

/**
 * Page handler for the administrator page to edit an existing signup.
 *
 * @param $signup
 *   The fully-loaded signup object to edit.
 *
 * @return
 *   The HTML to use for the signup edit page.
 */
function signup_edit_page($signup) {
  drupal_set_title(t('Edit signup'));
  return drupal_get_form('signup_edit_form', $signup, 'admin');
}

