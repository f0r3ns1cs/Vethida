import { AdjustIcon, DownloadIcon, HistoryIcon } from "@/components/icons";

const STEPS = [
  {
    icon: DownloadIcon,
    title: "Install it and see where you stand",
    body: "Vethida reads your hardware and your Windows settings and gives the PC a health score. Nothing is changed yet.",
  },
  {
    icon: AdjustIcon,
    title: "Apply what fits your PC",
    body: "Vethida marks a recommended set for this machine. Apply it in one go, or browse the settings and pick your own.",
  },
  {
    icon: HistoryIcon,
    title: "Review what changed",
    body: "The Restore page lists the changes Vethida made, and most can be undone. You can also create a Windows restore point before you start.",
  },
] as const;

export function HowItWorks() {
  return (
    <section className="band band--ruled">
      <div className="band__inner">
        <div className="section-head section-head--center">
          <h2 className="section-head__title">How it works</h2>
          <p className="section-head__deck">
            Look first, then change only what fits.
          </p>
        </div>

        <ol className="how">
          {STEPS.map(({ icon: Icon, title, body }, i) => (
            <li key={title} className="how__step">
              <div className="how__top">
                <span className="how__tile">
                  <Icon />
                </span>
                <span className="how__num">Step {i + 1}</span>
              </div>
              <h3 className="how__title">{title}</h3>
              <p className="how__body">{body}</p>
            </li>
          ))}
        </ol>
      </div>
    </section>
  );
}
