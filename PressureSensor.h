class RelayController {
public:
    RelayController();
    bool init(const std::vector<uint8_t>& relayPins);

    void setRelay(uint8_t index, bool state);
    void toggleRelay(uint8_t index);
    bool getRelayState(uint8_t index);

    // 新增
    void setRelayByPin(uint8_t pin, bool state);
    void toggleRelayByPin(uint8_t pin);
    bool getRelayStateByPin(uint8_t pin);

    void setAll(bool state);
    void offAll();
    void onAll();

private:
    bool _initialized;
    std::vector<uint8_t> _relayPins;
    std::vector<bool> _relayStates;
};