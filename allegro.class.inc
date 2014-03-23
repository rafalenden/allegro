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

  public $client;

  public function __construct() {
    $options = array(
      'connection_timeout' => 120,
      'features' => SOAP_SINGLE_ELEMENT_ARRAYS,
      'trace' => 1,
    );
    $SSL = TRUE;
    $this->serviceURL = ($SSL ? 'https' : 'http') . '://webapi.allegro.pl/service.php?wsdl';
    $this->client = new parent($this->serviceURL, $options);
    return $this;
  }

  //////////////////////////////////////////////////////////////////////////////
  //
  // Helper methods for Allegro WebAPI.
  //
  //////////////////////////////////////////////////////////////////////////////

  /**
   * Check if SOAP client is logged to Allegro WebAPI.
   *
   * @return bool
   */
  public function isLogged() {
    return $this->logged;
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

  /**
   * Get available Allegro Group auction websites.
   *
   * @return array
   *   List of services keyed by country ID.
   */
  public static function getSupportedWebsites() {
    return array(
      1   => 'allegro.pl',
      22  => 'allegro.by',
      34  => 'aukro.bg',
      56  => 'aukro.cz',
      107 => 'allegro.kz',
      168 => 'molotok.ru',
      181 => 'aukro.sk',
      209 => 'aukro.ua',
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

  //////////////////////////////////////////////////////////////////////////////
  //
  // Wrappers for WebAPI functions.
  //
  //////////////////////////////////////////////////////////////////////////////

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

    return $this->client->doCheckNewAuctionExt($params);
  }

  /**
   * Metoda pozwala na pobranie pełnego drzewa kategorii dostępnych we wskazanym kraju.
   * (http://allegro.pl/webapi/documentation.php/show/id,46)
   *
   * @return array
   */
  public function getCatsData() {
    return $this->client->doGetCatsData($this->countryCode, 0, $this->webapiKey);
  }

  public function getCatsDataCount() {
    $params = array(
      'countryId' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
    );
    return $this->client->doGetCatsDataCount($params);
  }

  public function getCatsDataLimit($offset = 0, $packageElement = 5000) {
    $params = array(
      'countryId' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
      'offset' => $offset,
      'packageElement' => $packageElement,
    );
    return $this->client->doGetCatsDataLimit($params);
  }

  public function getSellFormFields() {
    $params = array(
      'countryCode' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
    );
    return $this->client->doGetSellFormFieldsExt($params);
  }

  /**
   * Get number of fields in country.
   *
   * @return int
   *   Number of available fields in country.
   */
  public function getSellFormFieldsCount() {
    $params = array(
      'countryCode' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
      'offset' => 0,
      'packageElement' => 1,
    );
    $response = $this->client->doGetSellFormFieldsExtLimit($params);
    return $response->formFieldsCount;
  }

  public function getSellFormFieldsLimit($offset = 0, $packageElement = 50) {
    $params = array(
      'countryCode' => $this->countryCode,
      'localVersion' => 0,
      'webapiKey' => $this->webapiKey,
      'offset' => $offset,
      'packageElement' => $packageElement,
    );
    return $this->client->doGetSellFormFieldsExtLimit($params);
  }

  public function getVersion() {
    $result = $this->queryAllSysStatus();
    $versionKeys = array();
    foreach ($result->sysCountryStatus->item as $status) {
      $versionKeys[$status->countryId] = $status;
    }
    return $versionKeys[$this->countryCode]->verKey;
  }

  public function queryAllSysStatus() {
    $params = array(
      'countryId' => $this->countryCode,
      'webapiKey' => $this->webapiKey,
    );
    return $this->client->doQueryAllSysStatus($params);
  }

  public function querySysStatus($sysvar = 3) {
    $params = array(
      'sysvar' => $sysvar,
      'countryCode' => $this->countryCode,
      'webapiKey' => $this->webapiKey,
    );
    return $this->client->doQuerySysStatus($params);
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

    return $this->client->doNewAuctionExt($params);
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
      'previewOnly' => (int) $previewOnly,
    );

    return $this->client->doChangeItemFields($params);
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
      'finishCancelAllBids' => (int) $finishCancelAllBids,
      'finishCancelReason' => $finishCancelReason,
    );

    return $this->client->doFinishItems($params);
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

    return $this->client->doFinishItems($params);
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
      $this->session = $this->client->doLoginEnc($params);
    }
    else {
      $params['userPassword'] = $this->userPassword;
      $this->session = $this->client->doLogin($params);
    }

    $this->sid = $this->session->sessionHandlePart;
    $this->userId = $this->session->userId;
    $this->logged = TRUE;

    return $this;
  }

  /**
   * This method allows to list an offer based on already existing offers.
   *
   * Because of particular mechanism of relisting offers, offers identifiers
   * that are returned on output are in fact identifiers of offers serving as
   * base for listing new offers, therefore these are not identifiers of newly
   * listed offers. In order to check the identifier of a newly listed offer you
   * need to give it a local identifier while calling doSellSomeAgain and use it
   * in the doVerifyItem method.
   *
   * @see http://allegro.pl/webapi/documentation.php/show/id,1130?lang=en
   *
   * @param array $sellItemsArray
   *   Array of identifiers of offers to be relisted (max. 25).
   * @param int $sellStartingTime
   *   Offer listing date (in the Unix time format).
   *   When offers are to be listed right away, pass 0.
   * @param int $sellAuctionDuration
   *   Offer duration time expressed in days (acceptable values: 3, 5, 7, 10, 14
   *   and 21 - last for: Russia, Ukraine, Romania and Serbia).
   * @param int $sellOptions
   *   Parameter allowing to indicate additional actions to be taken after
   *   relisting offer (1 - remove source offers from the proper tab of My Allegro,
   *   2 - send e-mail confirming listing offer, 3 - both).
   * @param array $localIds
   *   Array of local identifiers to be additionally assigned to offers (range: 1-9999999999999).
   *
   * @return object
   *   Following properties are available:
   *   - itemsSellAgain: Array of structures containing information on relisted offers.
   *     - sellItemId: Identifier of an offer on which basis a new offer has been listed.
   *     - sellItemInfo: Information on costs related to listing an offer.
   *     - sellItemLocalId: Value of an assigned local identifier (to use in doVerifyItem).
   *   - itemsSellFailed: Array of structures containing information on not relisted offers.
   *     - sellItemId: Identifier of an offer on which basis a new offer was supposed to be listed.
   *     - sellFaultCode: Error code indicating a reason of the failure.
   *     - sellFaultString: Error message describing a reason of the failure.
   *   - itemsSellNotFound: Array of identifiers of not relisted offers (indicated offers were not created by a logged-in user or provided identifiers were incorrect).
   */
  public function sellSomeAgain(array $sellItemsArray, $sellStartingTime = 0, $sellAuctionDuration, $sellOptions, array $localIds = array()) {
    $params = array(
      'sessionHandle' => $this->sid,
      'sellItemsArray' => $sellItemsArray,
      'sellStartingTime' => $sellStartingTime,
      'sellAuctionDuration' => $sellAuctionDuration,
      'sellOptions' => $sellOptions,
      'localIds' => $localIds,
    );

    return $this->client->doSellSomeAgain($params);
  }

/**
 * This method allows for checking the correctness of listing an offer.
 *
 * It checks an offer created by the doNewAuctionExt method when a value of the
 * localId parameter has been passed while calling it from an account of a
 * logged-in user as well as a idenitifier of a relisted offer (created by using
 * doSellSomeAgain/doSellSomeAgainInShop when a value of the localId parameter
 * has been passed while calling them). localId value is always unique within a
 * given user account.
 *
 * @see http://allegro.pl/webapi/documentation.php/show/id,1075?lang=en
 *
 * @param int $localId
 *  Value of a local identifier that has been passed while listing an offer
 *   using the doNewAuctionExt/doSellSomeAgain/doSellSomeAgainShop method.
 *
 * @return object
 *   Following properties are available:
 *   - itemId: Offer identifier (or -1 when the provided local identifier is incorrect).
 *   - itemListed: Listing offer status (-1 - offer is not and will not be
 *       listed due to technical problems on Allegro or due to providing
 *       incorrect local identifier, 0 - pending to be listed, 1 - offer has
 *       been successfully listed, 2 - offer to be listed: a seller has set the
 *       time of listing in the sale form, 3 - offer to be relisted: relates to
 *       offers listed by using doSellSomeAgain/doSellSomeAgainShop).
 *   - itemStartingTime: Offer start date (in the Unix format; relates to all
 *       offers - planned, to be relisted and those already listed).
 */
  public function verifyItem($localId) {
    $params = array(
      'sessionHandle' => $this->sid,
      'localId' => $localId,
    );

    return $this->client->doVerifyItem($params);
  }

}

