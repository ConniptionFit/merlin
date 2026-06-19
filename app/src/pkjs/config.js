module.exports = [
  {
    type: 'heading',
    defaultValue: 'Merlin'
  },
  {
    type: 'text',
    defaultValue: 'Enter your Google Gemini API key. Merlin calls the Gemini API directly over HTTPS — no proxy server required.'
  },
  {
    type: 'section',
    items: [
      {
        type: 'input',
        messageKey: 'GeminiApiKey',
        label: 'Gemini API Key',
        attributes: {
          type: 'text',
          placeholder: 'AIza...'
        }
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save Settings'
  }
];
