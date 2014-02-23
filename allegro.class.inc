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
    try {
      $this->version = $this->getVersion();

      $params = array(
        'userLogin' => $this->userLogin,
        'countryCode' => $this->countryCode,
        'webapiKey' => $this->webapiKey,
        'localVersion' => $this->version,
      );

      if (self::isPasswordEncrypted()) {
        $this->session = $this->doLoginEnc($params);
        $params['userHashPassword'] = $this->userPassword;
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
    catch (SoapFault $e) {
      throw new SoapFault($e->faultcode, $e->faultstring);
    }
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
    try {
      $test = new Allegro();
      $test->userLogin = $login;
      $test->userPassword = $password;
      $test->webapiKey = $apiKey;
      $test->countryCode = $countryCode;
      $test->login();
      return TRUE;
    }
    catch (SoapFault $e) {
      throw new SoapFault($e->faultcode, $e->faultstring);
    }
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

