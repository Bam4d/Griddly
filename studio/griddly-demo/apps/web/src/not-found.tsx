import { Link } from "@tanstack/react-router";

import { useRootScreens } from "./root-screens";
import type { NotFoundScreenProps } from "./root-screens";

const PlainNotFound = ({ texts }: NotFoundScreenProps) => (
  <div className="flex flex-col items-start gap-2 p-8">
    <h1 className="font-semibold">{texts.notFound}</h1>
    <p>{texts.notFoundDescription}</p>
    <Link to="/" className="underline underline-offset-4">
      {texts.home}
    </Link>
  </div>
);

/**
 * The router's `defaultNotFoundComponent`, for a URL or a `notFound()` no route handles itself: the
 * contributed screen when there is one (`root-screens.tsx`), else a plain one.
 */
export const NotFound = () => {
  const { screens, texts } = useRootScreens();
  const Screen = screens.NotFound ?? PlainNotFound;

  return <Screen texts={texts} />;
};
