<?php

class Allegro extends SoapClient {

  public $session;
  public $sid;
  public $version;
  public $userId;
  public $userLogin;
  public $userPassword;
  public $countryCode = 1;
  public $webapiKey;
  public $logged = FALSE;

  public function __construct() {
    $options = array(
      'cache_wsdl' => WSDL_CACHE_NONE,
      'connection_timeout' => 120,
      'features' => SOAP_SINGLE_ELEMENT_ARRAYS,
      'trace' => 1,
    );
    $SSL = TRUE;
    $this->serviceURL = ($SSL ? 'https' : 'http') . '://webapi.allegro.pl/service.php?wsdl';
    return parent::__construct($this->serviceURL, $options);
  }

  /**
   * Log into WebAPI service.
   *
   * @return object
   *   Allegro client object.
   */
  public function login() {
    $this->version = $this->getVersion();

    $params = array(
      'userLogin' => $this->userLogin,
      'countryCode' => $this->countryCode,
      'webapiKey' => $this->webapiKey,
      'localVersion' => $this->version,
    );

    if (self::isPasswordEncrypted()) {
      $params['userHashPassword'] = $this->userPassword;
      $this->session = $this->doLoginEnc($params);
    }
    else {
      $params['userPassword'] = $this->userPassword;
      $this->session = $this->doLogin($params);
    }

    $this->sid = $this->session->sessionHandlePart;
    $this->userId = $this->session->userId;
    $this->logged = TRUE;

    return $this;
  }

  /**
   * Tests connection with WebAPI.
   *
   * @param string $login
   *   User login to WebAPI.
   * @param string $password
   *   User login to WebAPI.
   * @param string $apiKey
   *   Key to WebAPI.
   * @param string $countryCode
   *   Code of country to connect through WebAPI.
   *
   * @return object
   *   Allegro client object.
   */
  public static function testConnection($login, $password, $apiKey, $countryCode) {
    $test = new Allegro();
    $test->userLogin = $login;
    $test->userPassword = $password;
    $test->webapiKey = $apiKey;
    $test->countryCode = $countryCode;
    $test->login();
    return TRUE;
  }

  public function querySysStatus($sysvar = 3) {
    $params = array(
      'sysvar' => $sysvar,
      'countryCode' => $this->countryCode,
      'webapiKey' => $this->webapiKey,
    );
    return $this->doQuerySysStatus($params);
  }

  public function queryAllSysStatus() {
    $params = array(
      'countryId' => $this->countryCode,
      'webapiKey' => $this->webapiKey,
    );
    return $this->doQueryAllSysStatus($params);
  }

  public function getVersion() {
    $result = $this->queryAllSysStatus();
    $versionKeys = array();
    foreach ($result->sysCountryStatus->item as $status) {
      $versionKeys[$status->countryId] = $status;
    }
    return $versionKeys[$this->countryCode]->verKey;
  }

  public function isLogged() {
    return $this->logged;
  }

  /**
   * Metoda pozwala na pobranie pełnego drzewa kategorii dostępnych we wskazanym kraju.
   * (http://allegro.pl/webapi/documentation.php/show/id,46)
   *
   * @return array
   */
  public function getCatsData() {
    return $this->doGetCatsData($this->countryCode, 0, $this->webapiKey);
  }

  public function getCatsDataCount() {
    $params = array(
      'countryId' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
    );
    return $this->doGetCatsDataCount($params);
  }

  public function getCatsDataLimit($offset = 0, $packageElement = 5000) {
    $params = array(
      'countryId' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
      'offset' => $offset,
      'packageElement' => $packageElement,
    );
    return $this->doGetCatsDataLimit($params);
  }

  public function getSellFormFields() {
    $params = array(
      'countryCode' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
    );
    return $this->doGetSellFormFieldsExt($params);
  }

  public function getSellFormFieldsLimit($offset = 0, $packageElement = 50) {
    $params = array(
      'countryCode' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
      'offset' => $offset,
      'packageElement' => $packageElement,
    );
    return $this->doGetSellFormFieldsExtLimit($params);
  }

  public function checkItemDescription($description) {
    $params = array(
      'sessionId' => $this->sid,
      'descriptionContent' => $description,
    );

    return $this->__soapCall('doCheckItemDescription', $params);
  }

  public function checkNewAuction($fields) {
    $params = array(
      'sessionHandle' => $this->sid,
      'fields' => $fields,
    );

    return $this->doCheckNewAuctionExt($params);
  }

  public function newAuction($fields, $localId = 0) {
    $params = array(
      'sessionHandle' => $this->sid,
      'fields' => $fields,
      // 'itemTemplateId' => 0,
      'localId' => $localId,
      // 'itemTemplateCreate' => array(
        // 'itemTemplateOption' => 1,
        // 'itemTemplateName' => 'Test',
      // ),
    );

    return $this->doNewAuctionExt($params);
  }

  public function verifyItem($localId) {
    $params = array(
      'sessionHandle' => $this->sid,
      'localId' => $localId,
    );

    return $this->doVerifyItem($params);
  }

  /**
   * This method allows for comprehensive offer editing both listed and planned
   * to be listed.
   *
   * In order to add elements to the offer or change them, you need to pass data
   * of selected fields of the fieldsToModify structure.
   * To remove elements you need to pass data of selected fields of the
   * fieldsToRemove structure.
   * A list of fields available in the offer can be loaded by using the
   * doGetItemFields method.
   * When bids appear majority of fields is blocked to changes (e.g. description).
   * In such case you can add an additional description in the fid: 25.
   * This method allows for editing the offer and simulating it – the previewOnly
   * field sets the method's mechanics.
   *
   * @see http://allegro.pl/webapi/documentation.php/show/id,1190?lang=en
   *
   * @param int $itemId
   *   User's session identifier received using the doLogin(Enc) method.
   * @param array $fieldsToModify
   *   Identifier of an offer to be changed. The offer may be ongoing or planned
   *   to be listed.
   * @param array $fieldsToRemove
   *   Array of structures containing information on sale form fields that are
   *   to be changed or added.
   * @param bool $previewOnly
   *   Array of field identifiers (fids) that are to be removed.
   *
   * @return object
   *   Following properties are available:
   *   - changedItem: Structure containing information about a changed offer.
   *     - itemId: Offer identifier.
   *     - itemFields: Array of structures containing information about values of fields.
   *     - itemSurcharge: Array of structures of fees charged for a changed offer.
   *       - surchargeDescription: Full description of a fee.
   *       - surchargeAmount: Structure of fee amounts for a changed offer.
   *         - amountValue: Fee amount.
   *         - amountCurrencySign: Mark of a currency in which money is returned.
   */
  public function changeItemFields($itemId, $fieldsToModify = array(), $fieldsToRemove = array(), $previewOnly = FALSE) {
    $params = array(
      'sessionId' => $this->sid,
      'itemId' => $itemId,
      'fieldsToModify' => $fieldsToModify,
      'fieldsToRemove' => $fieldsToRemove,
      'previewOnly' => $previewOnly ? 1 : 0,
    );

    return $this->doChangeItemFields($params);
  }

/**
 * This method allows for closing the offer (both ongoing and scheduled for listing)
 * of a logged-in user before scheduled time (with or without cancellation of bids).
 *
 * @see http://allegro.pl/webapi/documentation.php/show/id,1122?lang=en
 *
 * @param int $finishItemId
 *   Offer identifier.
 * @param bool $finishCancelAllBids
 *   Information on whether bids are to be cancelled together with closing the offer
 *   (TRUE - yes, FALSE - no; default value is FALSE; only bids placed in the
 *   bidding process are cancelled).
 * @param string $finishCancelReason
 *   Reason for cancellation of bids. Required if finishCancelAllBids = TRUE.
 *
 * @return object
 *   Following properties are available:
 *   - finishValue: Operation result (1 - offer has been ended, 0 - offer has not been ended).
 */
  public function finishItem($finishItemId, $finishCancelAllBids = FALSE, $finishCancelReason = '') {
    $params = array(
      'sessionHandle' => $this->sid,
      'finishItemId' => $finishItemId,
      'finishCancelAllBids' => $finishCancelAllBids ? 1 : 0,
      'finishCancelReason' => $finishCancelReason,
    );

    return $this->doFinishItems($params);
  }

/**
 * This method allows for closing many offers (both ongoing and scheduled for listing)
 * of a logged-in user before scheduled time (with or without cancellation of bids).
 *
 * @see http://allegro.pl/webapi/documentation.php/show/id,1069?lang=en
 *
 * @param array $finishItemsList
 *   Arrays with structures containing information of offers to be closed (max. 25).
 *
 * @return object
 *   Following properties are available:
 *   - finishItemsSucceed: Array of identifiers of successfully ended offers.
 *   - finishItemsFailed: Arrays of structures containing information on offers that fail to end.
 *     - finishItemId: Offer identifier.
 *     - finishErrorCode: Error code indicating a reason of the failure.
 *     - finishErrorMessage: Error message describing a reason of the failure.
 */
  public function finishItems(array $finishItemsList) {
    $params = array(
      'sessionHandle' => $this->sid,
      'finishItemsList' => $finishItemsList,
    );

    return $this->doFinishItems($params);
  }

  /**
   * Get available Allegro Group auction websites.
   *
   * @return array
   *   List of services keyed by country ID.
   */
  public static function getCountries() {
    return array(
      1 => 'allegro.pl',
      228 => 'testwebapi.pl',
    );
  }

  /**
   * Check if hashing functions are available.
   *
   * @return bool
   *   TRUE when hashing is possible, FALSE otherwise.
   */
  public static function isPasswordEncrypted() {
    if ((function_exists('hash') && in_array('sha256', hash_algos()))
      || (function_exists('mhash') && is_int(MHASH_SHA256))) {
      return TRUE;
    }
    else {
      return FALSE;
    }
  }

}

