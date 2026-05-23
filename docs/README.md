# WireStack documentation

## Company onboarding PDF

**[WireStack-Company-Overview.pdf](WireStack-Company-Overview.pdf)** — self-contained onboarding for employees and partners: mission, vision, organic growth model, **vigil** product capabilities, roadmap, values, and how to get started.

Share this PDF as the primary “welcome to WireStack” artifact.

### Regenerate PDF and logos

```bash
python docs/generate_company_pdf.py
```

Outputs:

| File | Description |
|------|-------------|
| `docs/WireStack-Company-Overview.pdf` | Multi-page corporate overview |
| `docs/assets/wirestack-icon.png` | App / favicon-style mark |
| `docs/assets/wirestack-wordmark-dark.png` | Cover and slide headers |
| `docs/assets/wirestack-logo.svg` | Scalable logo |

Requires Python 3 with `fpdf2` and `Pillow` (`pip install fpdf2 pillow`).
